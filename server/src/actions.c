/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Execute pending actions - Improved version
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include <math.h>
#include "server.h"
#include "client.h"
#include "player.h"
#include "utils.h"
#include "actions.h"
#include "game.h"
#include <stdarg.h>
#include <stdbool.h>

static void send_response(int fd, const char *msg)
{
    send(fd, msg, strlen(msg), 0);
}

void broadcast_to_gui(server_t *srv, const char *msg)
{
    for (int i = 0; i < srv->client_count; i++) {
        if (srv->clients[i]->is_gui) {
            send(srv->clients[i]->socket_fd, msg, strlen(msg), 0);
        }
    }
}

static void sendf(int fd, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    send(fd, buf, strlen(buf), 0);
}

static void action_incantation_end(server_t *srv, client_t *cl, player_t *player)
{
    if (check_elevation_requirements(srv, player->x, player->y, player->level)) {
        perform_elevation(srv, player->x, player->y, player->level);
        sendf(cl->socket_fd, "Current level: %d\n", player->level);
        
        // Broadcast level change to GUI
        char msg[256];
        snprintf(msg, sizeof(msg), "plv #%d %d\n", player->id, player->level);
        broadcast_to_gui(srv, msg);
        
        // Check for victory condition
        int level8_count = 0;
        for (int i = 0; i < srv->player_count; i++) {
            if (srv->players[i]->level >= 8) {
                level8_count++;
            }
        }
        
        if (level8_count >= 6) {
            log_info("Victory! 6 players reached level 8");
            char victory_msg[256];
            snprintf(victory_msg, sizeof(victory_msg), "seg %s\n",
                srv->team_names[srv->players[0]->team_idx]);
            broadcast_to_gui(srv, victory_msg);
        }
    } else {
        send_response(cl->socket_fd, "ko\n");
        
        char msg[256];
        snprintf(msg, sizeof(msg), "pie %d %d 0\n", player->x, player->y);
        broadcast_to_gui(srv, msg);
    }
    player->is_incanting = 0;
}

void execute_pending_actions(server_t *srv)
{
    time_t now = time(NULL);

    for (int i = 0; i < srv->player_count; i++) {
        player_t *player = srv->players[i];
        if (player->pending_action[0] && now >= player->action_end_time && player->alive == true) {
            client_t *cl = NULL;

            // Find the client for this player
            for (int j = 0; j < srv->client_count; j++) {
                if (srv->clients[j]->id == player->client_idx) {
                    cl = srv->clients[j];
                    break;
                }
            }

            if (!cl) {
                // Clear the action if client is disconnected
                player->pending_action[0] = '\0';
                player->action_end_time = 0;
                continue;
            }

            // Execute the pending action
            if (strcmp(player->pending_action, "incantation_end") == 0) {
                action_incantation_end(srv, cl, player);
            }

            // Clear the action
            player->pending_action[0] = '\0';
            player->action_end_time = 0;
            
            // Notify GUI of position update if needed
            char pos_msg[256];
            snprintf(pos_msg, sizeof(pos_msg), "ppo #%d %d %d %d\n",
                player->id, player->x, player->y, player->orientation);
            broadcast_to_gui(srv, pos_msg);
        }
    }
    
    // Check for dead players and clean them up
    for (int i = srv->player_count - 1; i >= 0; i--) {
        if (!srv->players[i]->alive) {
            char death_msg[256];
            snprintf(death_msg, sizeof(death_msg), "pdi #%d\n", srv->players[i]->id);
            broadcast_to_gui(srv, death_msg);
            
            // Find and disconnect the client
            for (int j = 0; j < srv->client_count; j++) {
                if (srv->clients[j]->id == srv->players[i]->client_idx) {
                    send_response(srv->clients[j]->socket_fd, "dead\n");
                    break;
                }
            }
            
            // Remove player from array
            player_destroy(srv->players[i]);
            for (int j = i + 1; j < srv->player_count; j++) {
                srv->players[j - 1] = srv->players[j];
            }
            srv->player_count--;
        }
    }
}