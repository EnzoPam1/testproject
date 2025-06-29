/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Network implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include "network.h"
#include "client.h"
#include "server.h"
#include "utils.h"

network_t *network_create(uint16_t port)
{
    network_t *net = calloc(1, sizeof(network_t));
    if (!net) return NULL;

    // Create listen socket
    net->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (net->listen_fd < 0) {
        free(net);
        return NULL;
    }

    // Allow reuse
    int opt = 1;
    setsockopt(net->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(net->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(net->listen_fd);
        free(net);
        return NULL;
    }

    // Listen
    if (listen(net->listen_fd, 128) < 0) {
        close(net->listen_fd);
        free(net);
        return NULL;
    }

    // Initialize poll
    net->poll_capacity = 16;
    net->poll_fds = calloc(net->poll_capacity, sizeof(struct pollfd));
    net->poll_fds[0].fd = net->listen_fd;
    net->poll_fds[0].events = POLLIN;
    net->poll_count = 1;

    // Initialize clients
    net->client_capacity = 16;
    net->clients = calloc(net->client_capacity, sizeof(client_t *));

    return net;
}

void network_destroy(network_t *net)
{
    if (!net) return;

    // Close all clients
    for (int i = 0; i < net->client_count; i++) {
        if (net->clients[i]) {
            close(net->clients[i]->fd);
            client_destroy(net->clients[i]);
        }
    }

    // Close listen socket
    close(net->listen_fd);

    // Free memory
    free(net->poll_fds);
    free(net->clients);
    free(net);
}

int network_poll(network_t *net, int timeout)
{
    return poll(net->poll_fds, net->poll_count, timeout);
}

client_t *network_accept_client(network_t *net)
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int fd = accept(net->listen_fd, (struct sockaddr *)&addr, &addr_len);
    if (fd < 0) return NULL;

    // Create client
    client_t *client = client_create(fd);
    if (!client) {
        close(fd);
        return NULL;
    }

    // Expand arrays if needed
    if (net->client_count >= net->client_capacity) {
        net->client_capacity *= 2;
        net->clients = realloc(net->clients, net->client_capacity * sizeof(client_t *));
    }

    if (net->poll_count >= net->poll_capacity) {
        net->poll_capacity *= 2;
        net->poll_fds = realloc(net->poll_fds, net->poll_capacity * sizeof(struct pollfd));
    }

    // Add client
    net->clients[net->client_count++] = client;
    net->poll_fds[net->poll_count].fd = fd;
    net->poll_fds[net->poll_count].events = POLLIN;
    net->poll_count++;

    return client;
}

void network_disconnect_client(network_t *net, client_t *client)
{
    // Find client index
    int index = -1;
    for (int i = 0; i < net->client_count; i++) {
        if (net->clients[i] == client) {
            index = i;
            break;
        }
    }

    if (index < 0) return;

    // Close and destroy
    close(client->fd);
    client_destroy(client);

    // Remove from arrays
    for (int i = index; i < net->client_count - 1; i++) {
        net->clients[i] = net->clients[i + 1];
    }
    net->client_count--;

    // Remove from poll
    for (int i = index + 1; i < net->poll_count - 1; i++) {
        net->poll_fds[i] = net->poll_fds[i + 1];
    }
    net->poll_count--;

    log_info("Client disconnected");
}

void network_send(client_t *client, const char *format, ...)
{
    char buffer[BUFFER_SIZE];
    va_list args;
    
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len <= 0 || len >= BUFFER_SIZE) return;

    // Try to send immediately
    int sent = send(client->fd, buffer, len, MSG_DONTWAIT);
    if (sent < len) {
        // Buffer the rest
        int remaining = len - (sent > 0 ? sent : 0);
        if (client->output_size + remaining < BUFFER_SIZE) {
            memcpy(client->output_buffer + client->output_size, 
                   buffer + (sent > 0 ? sent : 0), remaining);
            client->output_size += remaining;
        }
    }
}

void network_send_to_all_gui(network_t *net, const char *format, ...)
{
    char buffer[BUFFER_SIZE];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (int i = 0; i < net->client_count; i++) {
        if (net->clients[i]->type == CLIENT_GUI) {
            network_send(net->clients[i], "%s", buffer);
        }
    }
}

char *network_read_line(client_t *client)
{
    // Receive data
    char temp[BUFFER_SIZE];
    int received = recv(client->fd, temp, 
                       BUFFER_SIZE - client->input_size - 1, MSG_DONTWAIT);

    if (received > 0) {
        // Add to buffer
        memcpy(client->input_buffer + client->input_size, temp, received);
        client->input_size += received;
        client->input_buffer[client->input_size] = '\0';
    } else if (received == 0 || (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        // Connection closed or error
        return NULL;
    }

    // Look for newline
    char *newline = strchr(client->input_buffer, '\n');
    if (!newline) return NULL;

    // Extract line
    size_t line_len = newline - client->input_buffer;
    char *line = malloc(line_len + 1);
    memcpy(line, client->input_buffer, line_len);
    line[line_len] = '\0';

    // Remove from buffer
    memmove(client->input_buffer, newline + 1, 
            client->input_size - line_len - 1);
    client->input_size -= line_len + 1;

    return str_trim(line);
}

void network_process_client_data(server_t *server, client_t *client)
{
    // Send any buffered output
    if (client->output_size > 0) {
        int sent = send(client->fd, client->output_buffer, 
                       client->output_size, MSG_DONTWAIT);
        if (sent > 0) {
            memmove(client->output_buffer, client->output_buffer + sent,
                    client->output_size - sent);
            client->output_size -= sent;
        }
    }

    // Read input
    char *line;
    while ((line = network_read_line(client)) != NULL) {
        if (strlen(line) == 0) {
            free(line);
            continue;
        }

        handle_client_command(server, client, line);
        free(line);

        // Check if client was disconnected
        bool still_connected = false;
        for (int i = 0; i < server->network->client_count; i++) {
            if (server->network->clients[i] == client) {
                still_connected = true;
                break;
            }
        }
        if (!still_connected) break;
    }
}