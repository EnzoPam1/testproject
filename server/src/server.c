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
#include "server.h"
#include "game.h"
#include "network.h"
#include "timer.h"
#include "client.h"
#include "player.h"
#include "utils.h"
#include "command.h"
#include "gui_protocol.h"

static config_t *parse_arguments(int argc, char **argv)
{
    config_t *config = calloc(1, sizeof(config_t));
    if (!config) return NULL;

    int opt;
    char *names = NULL;
    config->freq = 100;  // Default frequency

    while ((opt = getopt(argc, argv, "p:x:y:n:c:f:")) != -1) {
        switch (opt) {
            case 'p': config->port = atoi(optarg); break;
            case 'x': config->width = atoi(optarg); break;
            case 'y': config->height = atoi(optarg); break;
            case 'n': names = strdup(optarg); break;
            case 'c': config->clients_nb = atoi(optarg); break;
            case 'f': config->freq = atoi(optarg); break;
            default:
                free(config);
                free(names);
                return NULL;
        }
    }

    // Validate and parse team names
    if (!names || !config->port || !config->width || !config->height || !config->clients_nb) {
        free(config);
        free(names);
        return NULL;
    }

    config->team_names = str_split(names, ' ');
    config->team_count = str_array_len(config->team_names);
    free(names);

    if (config->team_count == 0) {
        str_array_free(config->team_names);
        free(config);
        return NULL;
    }

    return config;
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

    // Create timer
    server->timer = timer_create(server->config->freq);
    if (!server->timer) {
        log_error("Failed to create timer");
        server_destroy(server);
        return NULL;
    }

    server->running = true;
    server->start_time = time(NULL);

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
    if (server->timer) timer_destroy(server->timer);
    
    if (server->config) {
        if (server->config->team_names)
            str_array_free(server->config->team_names);
        free(server->config);
    }

    free(server);
}

static void handle_new_connection(server_t *server)
{
    client_t *client = network_accept_client(server->network);
    if (client) {
        network_send(client, "WELCOME\n");
        log_info("New client connected (fd=%d)", client->fd);
    }
}

static void handle_client_authentication(server_t *server, client_t *client, const char *data)
{
    // Check for GUI
    if (strcmp(data, "GRAPHIC") == 0) {
        client->type = CLIENT_GUI;
        client->state = STATE_PLAYING;
        gui_send_initial_data(server, client);
        log_info("GUI client authenticated");
        return;
    }

    // Check for AI team
    team_t *team = game_get_team_by_name(server->game, data);
    if (!team) {
        network_send(client, "ko\n");
        network_disconnect_client(server->network, client);
        return;
    }

    int slots = team_available_slots(team);
    if (slots <= 0) {
        network_send(client, "0\n");
        network_disconnect_client(server->network, client);
        return;
    }

    // Create player
    player_t *player = game_add_player(server->game, client->fd, data);
    if (!player) {
        network_send(client, "ko\n");
        network_disconnect_client(server->network, client);
        return;
    }

    // Setup client
    client->type = CLIENT_AI;
    client->state = STATE_PLAYING;
    client->player_id = player->id;
    client->team_id = team->id;

    // Send connection response
    network_send(client, "%d\n%d %d\n", slots - 1, 
                 server->config->width, server->config->height);

    // Notify GUI
    gui_notify_player_connect(server, player);

    log_info("Player %d joined team '%s' at (%d,%d)", 
             player->id, team->name, player->x, player->y);
}

static void handle_client_command(server_t *server, client_t *client, const char *command)
{
    if (client->state == STATE_CONNECTING) {
        handle_client_authentication(server, client, command);
    } else if (client->state == STATE_PLAYING) {
        command_process(server, client, command);
    }
}

static void process_completed_actions(server_t *server)
{
    for (int i = 0; i < server->game->player_count; i++) {
        player_t *player = server->game->players[i];
        
        if (player->action.command && player_action_done(player, server->config->freq)) {
            // Find client
            client_t *client = NULL;
            for (int j = 0; j < server->network->client_count; j++) {
                if (server->network->clients[j]->fd == player->client_id) {
                    client = server->network->clients[j];
                    break;
                }
            }

            if (client) {
                // Action completed, process next command
                client_command_done(client);
                
                // Execute next command if any
                char *next = client_get_current_command(client);
                if (next && !player->is_dead) {
                    command_execute(server, client, player, next);
                }
            }

            free(player->action.command);
            player->action.command = NULL;
        }
    }
}

int server_run(server_t *server)
{
    log_info("Server running on port %d", server->config->port);

    while (server->running) {
        // Calculate timeout
        int timeout = timer_get_timeout(server->timer);
        
        // Poll network
        int activity = network_poll(server->network, timeout);
        if (activity < 0) {
            log_error("Poll error");
            continue;
        }

        // Handle new connections
        if (server->network->poll_fds[0].revents & POLLIN) {
            handle_new_connection(server);
        }

        // Handle client data
        for (int i = 0; i < server->network->client_count; i++) {
            client_t *client = server->network->clients[i];
            if (server->network->poll_fds[i + 1].revents & POLLIN) {
                network_process_client_data(server, client);
            }
        }

        // Update timer
        timer_update(server->timer);

        // Game tick
        if (timer_should_tick(server->timer)) {
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