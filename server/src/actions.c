/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Execute pending actions - CORRECTED VERSION
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
    char formatted[1024];
    int len;
    
    // Ensure response has proper line termination
    if (msg[strlen(msg) - 1] != '\n') {
        len = snprintf(formatted, sizeof(formatted), "%s\n", msg);
    } else {
        len = snprintf(formatted, sizeof(formatted), "%s", msg);
    }
    
    if (len > 0 && len < sizeof(formatted)) {
        send(fd, formatted, len, 0);
        log_info("ACTION_RESPONSE[%d]: %.*s", fd, len - 1, formatted); // Log without \n
    }
}

void broadcast_to_gui(server_t *srv, const char *msg)
{
    for (int i = 0; i < srv->client_count; i++) {
        if (srv->clients[i]->is_gui) {
            // Ensure message has proper termination
            char formatted[1024];
            int len;
            
            if (msg[strlen(msg) - 1] != '\n') {
                len = snprintf(formatted, sizeof(formatted), "%s\n", msg);
            } else {
                len = snprintf(formatted, sizeof(formatted), "%s", msg);
            }
            
            if (len > 0 && len < sizeof(formatted)) {
                send(srv->clients[i]->socket_fd, formatted, len, 0);
            }
        }
    }
}

static void sendf(int fd, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    
    // Ensure proper line termination
    if (len > 0 && len < sizeof(buf) - 1 && buf[len - 1] != '\n') {
        buf[len] = '\n';
        buf[len + 1] = '\0';
        len++;
    }
    
    if (len > 0) {
        send(fd, buf, len, 0);
        log_info("ACTION_SENDF[%d]: %.*s", fd, len - 1, buf); // Log without \n
    }
}

static void generate_look_response(server_t *srv, player_t *player, char *response)
{
    char tile_content[512];
    int vision_size = player->level + 1;
    int first = 1;

    strcpy(response, "[");
    for (int row = 0; row < vision_size; row++) {
        int tiles_in_row = 2 * row + 1;
        int start_offset = -row;

        for (int col = 0; col < tiles_in_row; col++) {
            if (!first)
                strcat(response, ",");
            first = 0;

            int dx = 0, dy = 0;
            int offset = start_offset + col;

            switch (player->orientation) {
                case NORTH:
                    dx = offset;
                    dy = -row;
                    break;
                case SOUTH:
                    dx = -offset;
                    dy = row;
                    break;
                case EAST:
                    dx = row;
                    dy = offset;
                    break;
                case WEST:
                    dx = -row;
                    dy = -offset;
                    break;
            }

            int real_x = (player->x + dx + srv->width) % srv->width;
            int real_y = (player->y + dy + srv->height) % srv->height;
            tile_t *tile = &srv->map[real_y][real_x];
            tile_content[0] = '\0';

            for (int i = 0; i < srv->player_count; i++) {
                if (srv->players[i]->x == real_x &&
                    srv->players[i]->y == real_y) {
                    if (strlen(tile_content) > 0)
                        strcat(tile_content, " ");
                    strcat(tile_content, "player");
                }
            }

            const char *res_names[] = {"food", "linemate", "deraumere",
                "sibur", "mendiane", "phiras", "thystame"};
            for (int r = 0; r < 7; r++) {
                int count = (r == 0) ? tile->food : tile->stones[r - 1];
                for (int c = 0; c < count; c++) {
                    if (strlen(tile_content) > 0)
                        strcat(tile_content, " ");
                    strcat(tile_content, res_names[r]);
                }
            }
            strcat(response, tile_content);
        }
    }
    strcat(response, "]");
}

static int calculate_sound_direction(server_t *srv, player_t *sender,
    player_t *receiver)
{
    int dx = sender->x - receiver->x;
    int dy = sender->y - receiver->y;

    if (abs(dx) > srv->width / 2)
        dx = dx > 0 ? dx - srv->width : dx + srv->width;
    if (abs(dy) > srv->height / 2)
        dy = dy > 0 ? dy - srv->height : dy + srv->height;

    if (dx == 0 && dy == 0)
        return 0;

    double angle = atan2(dy, dx) * 180 / M_PI;
    angle = fmod(angle + 360, 360);

    int dir_offset = ((int)((angle + 22.5) / 45) % 8);
    int direction_map[4][8] = {
        {3, 2, 1, 4, 5, 6, 7, 8},
        {5, 4, 3, 2, 1, 8, 7, 6},
        {7, 6, 5, 4, 3, 2, 1, 8},
        {1, 8, 7, 6, 5, 4, 3, 2}
    };

    return direction_map[receiver->orientation - 1][dir_offset];
}

static void action_forward_end(server_t *srv, client_t *cl, player_t *player)
{
    player_move_forward(player, srv);
    send_response(cl->socket_fd, "ok");

    char msg[256];
    snprintf(msg, sizeof(msg), "ppo #%d %d %d %d\n",
        player->id, player->x, player->y, player->orientation);
    broadcast_to_gui(srv, msg);
}

static void action_right_end(server_t *srv, client_t *cl, player_t *player)
{
    player_turn(player, 1);
    send_response(cl->socket_fd, "ok");

    char msg[256];
    snprintf(msg, sizeof(msg), "ppo #%d %d %d %d\n",
        player->id, player->x, player->y, player->orientation);
    broadcast_to_gui(srv, msg);
}

static void action_left_end(server_t *srv, client_t *cl, player_t *player)
{
    player_turn(player, -1);
    send_response(cl->socket_fd, "ok");

    char msg[256];
    snprintf(msg, sizeof(msg), "ppo #%d %d %d %d\n",
        player->id, player->x, player->y, player->orientation);
    broadcast_to_gui(srv, msg);
}

static void action_look_end(server_t *srv, client_t *cl, player_t *player)
{
    char response[4096];
    generate_look_response(srv, player, response);
    send_response(cl->socket_fd, response);
}

static void action_inventory_end(server_t *srv, client_t *cl, player_t *player)
{
    (void)srv;
    char response[512];
    snprintf(response, sizeof(response),
        "[food %d, linemate %d, deraumere %d, sibur %d, mendiane %d, phiras %d, thystame %d]",
        player->inventory[0], player->inventory[1], player->inventory[2],
        player->inventory[3], player->inventory[4], player->inventory[5],
        player->inventory[6]);
    send_response(cl->socket_fd, response);
}

static void action_take_end(server_t *srv, client_t *cl, player_t *player,
    int res)
{
    tile_t *tile = &srv->map[player->y][player->x];

    if (player_take_resource(player, tile, res)) {
        send_response(cl->socket_fd, "ok");

        char msg[256];
        snprintf(msg, sizeof(msg), "pgt #%d %d\n", player->id, res);
        broadcast_to_gui(srv, msg);

        snprintf(msg, sizeof(msg), "bct %d %d %d %d %d %d %d %d %d\n",
            player->x, player->y, tile->food, tile->stones[0], tile->stones[1],
            tile->stones[2], tile->stones[3], tile->stones[4], tile->stones[5]);
        broadcast_to_gui(srv, msg);
    } else {
        send_response(cl->socket_fd, "ko");
    }
}

static void action_set_end(server_t *srv, client_t *cl, player_t *player,
    int res)
{
    tile_t *tile = &srv->map[player->y][player->x];

    if (player_drop_resource(player, tile, res)) {
        send_response(cl->socket_fd, "ok");

        char msg[256];
        snprintf(msg, sizeof(msg), "pdr #%d %d\n", player->id, res);
        broadcast_to_gui(srv, msg);

        snprintf(msg, sizeof(msg), "bct %d %d %d %d %d %d %d %d %d\n",
            player->x, player->y, tile->food, tile->stones[0], tile->stones[1],
            tile->stones[2], tile->stones[3], tile->stones[4], tile->stones[5]);
        broadcast_to_gui(srv, msg);
    } else {
        send_response(cl->socket_fd, "ko");
    }
}

static void action_broadcast_end(server_t *srv, client_t *cl, player_t *player,
    const char *message)
{
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i] == player)
            continue;

        int direction = calculate_sound_direction(srv, player, srv->players[i]);
        client_t *recv_cl = NULL;

        for (int j = 0; j < srv->client_count; j++) {
            if (srv->clients[j]->id == srv->players[i]->client_idx) {
                recv_cl = srv->clients[j];
                break;
            }
        }

        if (recv_cl) {
            sendf(recv_cl->socket_fd, "message %d, %s", direction, message);
        }
    }

    send_response(cl->socket_fd, "ok");

    char msg[512];
    snprintf(msg, sizeof(msg), "pbc #%d %s\n", player->id, message);
    broadcast_to_gui(srv, msg);
}

static void action_fork_end(server_t *srv, client_t *cl, player_t *player)
{
    srv->slots_remaining[player->team_idx]++;
    send_response(cl->socket_fd, "ok");

    char msg[256];
    snprintf(msg, sizeof(msg), "pfk #%d\n", player->id);
    broadcast_to_gui(srv, msg);

    static int egg_id = 1;
    snprintf(msg, sizeof(msg), "enw #%d #%d %d %d\n",
        egg_id++, player->id, player->x, player->y);
    broadcast_to_gui(srv, msg);
}

static void action_eject_end(server_t *srv, client_t *cl, player_t *player)
{
    int ejected = 0;

    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i] != player &&
            srv->players[i]->x == player->x &&
            srv->players[i]->y == player->y) {

            player_move_forward(srv->players[i], srv);

            client_t *ejected_cl = NULL;
            for (int j = 0; j < srv->client_count; j++) {
                if (srv->clients[j]->id == srv->players[i]->client_idx) {
                    ejected_cl = srv->clients[j];
                    break;
                }
            }

            if (ejected_cl) {
                int dir = calculate_sound_direction(srv, player, srv->players[i]);
                sendf(ejected_cl->socket_fd, "eject: %d", dir);
            }

            char msg[256];
            snprintf(msg, sizeof(msg), "pex #%d\n", srv->players[i]->id);
            broadcast_to_gui(srv, msg);

            ejected = 1;
        }
    }

    send_response(cl->socket_fd, ejected ? "ok" : "ko");
}

static void action_incantation_end(server_t *srv, client_t *cl,
    player_t *player)
{
    log_info("Completing incantation for player #%d at (%d,%d) level %d", 
             player->id, player->x, player->y, player->level);
             
    if (check_elevation_requirements(srv, player->x, player->y, player->level)) {
        perform_elevation(srv, player->x, player->y, player->level);
        sendf(cl->socket_fd, "Current level: %d", player->level);
        log_info("Player #%d successfully elevated to level %d", player->id, player->level);
    } else {
        send_response(cl->socket_fd, "ko");
        log_info("Player #%d incantation failed - requirements not met", player->id);
        
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

            for (int j = 0; j < srv->client_count; j++) {
                if (srv->clients[j]->id == player->client_idx) {
                    cl = srv->clients[j];
                    break;
                }
            }

            if (!cl) {
                log_info("No client found for player #%d action completion", player->id);
                continue;
            }

            char action[64], arg[256];
            sscanf(player->pending_action, "%s %[^\n]", action, arg);

            log_info("Executing pending action '%s' for player #%d", action, player->id);

            if (strcmp(action, "incantation_end") == 0)
                action_incantation_end(srv, cl, player);

            player->pending_action[0] = '\0';
            player->action_end_time = 0;
        }
    }
}