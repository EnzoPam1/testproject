/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Client implementation
*/

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include "client.h"
#include "utils.h"

client_t *client_create(int fd)
{
    client_t *client = calloc(1, sizeof(client_t));
    if (!client) return NULL;

    client->fd = fd;
    client->type = CLIENT_UNKNOWN;
    client->state = STATE_CONNECTING;
    client->player_id = -1;
    client->team_id = -1;
    client->current_action.is_active = false;
    client->current_action.command = NULL;

    return client;
}

void client_destroy(client_t *client)
{
    if (!client) return;

    // Free command queue
    for (int i = 0; i < client->cmd_queue.count; i++) {
        free(client->cmd_queue.commands[i]);
    }

    // Free current action
    if (client->current_action.command) {
        free(client->current_action.command);
    }

    free(client);
}

bool client_add_command(client_t *client, const char *command)
{
    if (client->cmd_queue.count >= MAX_COMMANDS) {
        return false;  // Queue full
    }

    client->cmd_queue.commands[client->cmd_queue.count] = strdup(command);
    client->cmd_queue.count++;
    
    return true;
}

char *client_get_current_command(client_t *client)
{
    if (client->cmd_queue.executing >= client->cmd_queue.count) {
        return NULL;
    }
    
    return client->cmd_queue.commands[client->cmd_queue.executing];
}

void client_command_done(client_t *client)
{
    if (client->cmd_queue.executing >= client->cmd_queue.count) {
        return;
    }

    // Free executed command
    free(client->cmd_queue.commands[client->cmd_queue.executing]);

    // Shift commands
    for (int i = client->cmd_queue.executing; i < client->cmd_queue.count - 1; i++) {
        client->cmd_queue.commands[i] = client->cmd_queue.commands[i + 1];
    }

    client->cmd_queue.count--;
    
    // Keep executing index at 0 to process next command
    client->cmd_queue.executing = 0;
}

bool client_can_send_command(client_t *client)
{
    return client->cmd_queue.count < MAX_COMMANDS;
}

void client_start_action(client_t *client, const char *command, int duration)
{
    if (client->current_action.command) {
        free(client->current_action.command);
    }
    
    client->current_action.command = strdup(command);
    client->current_action.duration = duration;
    gettimeofday(&client->current_action.start_time, NULL);
    client->current_action.is_active = true;
}

bool client_action_done(client_t *client, int freq)
{
    if (!client->current_action.is_active) return false;
    
    struct timeval now;
    gettimeofday(&now, NULL);
    
    double elapsed = (now.tv_sec - client->current_action.start_time.tv_sec) +
                    (now.tv_usec - client->current_action.start_time.tv_usec) / 1000000.0;
    
    double required_time = (double)client->current_action.duration / freq;
    
    return elapsed >= required_time;
}

void client_send(client_t *client, const char *format, ...)
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

char *client_read_line(client_t *client)
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

    // Trim whitespace
    char *trimmed = line;
    while (isspace(*trimmed)) trimmed++;
    
    if (*trimmed == 0) {
        free(line);
        return NULL;
    }
    
    char *end = trimmed + strlen(trimmed) - 1;
    while (end > trimmed && isspace(*end)) end--;
    end[1] = '\0';
    
    if (trimmed != line) {
        char *result = strdup(trimmed);
        free(line);
        return result;
    }

    return line;
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
    while ((line = client_read_line(client)) != NULL) {
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