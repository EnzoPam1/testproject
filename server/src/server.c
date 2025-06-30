/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Server implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "server.h"
#include "game.h"
#include "network.h"
#include "client.h"
#include "player.h"
#include "utils.h"
#include "command.h"
#include "gui_protocol.h"

server_t *g_server = NULL;

static config_t *parse_arguments(int argc, char **argv)
{
    config_t *config = calloc(1, sizeof(config_t));
    if (!config) return NULL;

    int opt;
    char **names = NULL;
    int name_count = 0;
    config->freq = 100;  // Default frequency

    while ((opt = getopt(argc, argv, "p:x:y:n:c:f:")) != -1) {
        switch (opt) {
            case 'p': config->port = atoi(optarg); break;
            case 'x': config->width = atoi(optarg); break;
            case 'y': config->height = atoi(optarg); break;
            case 'n': 
                // Count remaining team names
                name_count = argc - optind + 1;
                names = malloc(name_count * sizeof(char*));
                names[0] = strdup(optarg);
                for (int i = 1; i < name_count && optind < argc; i++) {
                    if (argv[optind][0] == '-') break;
                    names[i] = strdup(argv[optind++]);
                }
                config->team_names = names;
                config->team_count = name_count;
                break;
            case 'c': config->clients_nb = atoi(optarg); break;
            case 'f': config->freq = atoi(optarg); break;
            default:
                free(config);
                return NULL;
        }
    }

    // Validate required parameters
    if (!config->port || !config->width || !config->height || 
        !config->clients_nb || !config->team_names || config->team_count == 0) {
        if (config->team_names) {
            for (int i = 0; i < config->team_count; i++) {
                free(config->team_names[i]);
            }
            free(config->team_names);
        }
        free(config);
        return NULL;
    }

    return config;
}

static network_t *network_create(uint16_t port)
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

static void network_destroy(network_t *net)
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

static client_t *network_accept_client(server_t *server)
{
    network_t *net = server->network;
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

    // Send welcome message
    client_send(client, "WELCOME\n");

    return client;
}

static void network_disconnect_client(server_t *server, client_t *client)
{
    network_t *net = server->network;
    
    // Find client index
    int index = -1;
    for (int i = 0; i < net->client_count; i++) {
        if (net->clients[i] == client) {
            index = i;
            break;
        }
    }

    if (index < 0) return;

    // Remove player if AI client
    if (client->type == CLIENT_AI && client->player_id >= 0) {
        player_t *player = game_get_player_by_id(server->game, client->player_id);
        if (player) {
            gui_notify_player_death(server, client->player_id);
            game_remove_player(server->game, client->player_id);
        }
    }

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

server_t *server_create(int argc, char **argv)
{
    server_t *server = calloc(1, sizeof(server_t));
    if (!server) return NULL;

    // Parse configuration
    server->config = parse_arguments(argc, argv);
    if (!server->config) {
        log_error("Invalid arguments");
        free(server);
        return NULL;
    }

    // Create game
    server->game = game_create(server->config->width, server->config->height,
                               server->config->team_names, server->config->team_count,
                               server->config->clients_nb);
    if (!server->game) {
        log_error("Failed to create game");
        server_destroy(server);
        return NULL;
    }

    // Create network
    server->network = network_create(server->config->port);
    if (!server->network) {
        log_error("Failed to create network on port %d", server->config->port);
        server_destroy(server);
        return NULL;
    }

    server->running = true;
    gettimeofday(&server->start_time, NULL);
    gettimeofday(&server->last_tick, NULL);
    server->tick_accumulator = 0.0;

    log_info("Server created - Port: %d, Map: %dx%d, Teams: %d, Freq: %d",
             server->config->port, server->config->width, server->config->height,
             server->config->team_count, server->config->freq);

    return server;
}

void server_destroy(server_t *server)
{
    if (!server) return;

    if (server->game) game_destroy(server->game);
    if (server->network) network_destroy(server->network);
    
    if (server->config) {
        if (server->config->team_names) {
            for (int i = 0; i < server->config->team_count; i++) {
                free(server->config->team_names[i]);
            }
            free(server->config->team_names);
        }
        free(server->config);
    }

    free(server);
}

static double get_time_diff(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec) / 1000000.0;
}

static void process_completed_actions(server_t *server)
{
    for (int i = 0; i < server->network->client_count; i++) {
        client_t *client = server->network->clients[i];
        
        if (client->type == CLIENT_AI && client->current_action.is_active) {
            if (client_action_done(client, server->config->freq)) {
                // Action completed, process next command
                client->current_action.is_active = false;
                if (client->current_action.command) {
                    free(client->current_action.command);
                    client->current_action.command = NULL;
                }
                
                client_command_done(client);
                
                // Execute next command if any
                char *next = client_get_current_command(client);
                if (next) {
                    player_t *player = game_get_player_by_id(server->game, client->player_id);
                    if (player && !player->is_dead) {
                        command_execute(server, client, player, next);
                    }
                }
            }
        }
    }
}

int server_run(server_t *server)
{
    log_info("Server running on port %d", server->config->port);

    while (server->running) {
        // Calculate timeout for next tick
        double tick_duration = 1.0 / server->config->freq;
        int timeout = (int)(tick_duration * 1000);
        
        // Poll network
        int activity = poll(server->network->poll_fds, server->network->poll_count, timeout);
        if (activity < 0) {
            if (errno == EINTR) continue;
            log_error("Poll error: %s", strerror(errno));
            continue;
        }

        // Handle new connections
        if (server->network->poll_fds[0].revents & POLLIN) {
            network_accept_client(server);
        }

        // Handle client data
        for (int i = 0; i < server->network->client_count; i++) {
            client_t *client = server->network->clients[i];
            if (server->network->poll_fds[i + 1].revents & POLLIN) {
                network_process_client_data(server, client);
            }
        }

        // Game tick
        struct timeval now;
        gettimeofday(&now, NULL);
        double elapsed = get_time_diff(&server->last_tick, &now);
        server->tick_accumulator += elapsed * server->config->freq;
        server->last_tick = now;

        if (server->tick_accumulator >= 1.0) {
            server->tick_accumulator -= 1.0;
            game_tick(server->game, server->config->freq);
            
            // Check victory
            if (game_check_victory(server->game)) {
                gui_notify_game_end(server, server->game->winning_team);
                log_info("Game won by team %s!", server->game->winning_team);
                server->running = false;
            }
        }

        // Process completed actions
        process_completed_actions(server);
    }

    log_info("Server shutting down");
    return 0;
}

void server_stop(server_t *server)
{
    if (server) {
        server->running = false;
    }
}