#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <math.h>
#include <time.h>
#include "commands.h"
#include "server.h"
#include "client.h"
#include "player.h"
#include "utils.h"
#include "actions.h"

// Helper to map resource name to enum
static resource_t resource_from_name(const char *name) {
    const char *res_names[] = {"food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"};
    for (int i = 0; i < 7; i++) {
        if (strcmp(name, res_names[i]) == 0) return (resource_t)i;
    }
    return -1;
}

static void send_response(int fd, const char *msg) {
    send(fd, msg, strlen(msg), 0);
}

static void sendf(int fd, const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    send(fd, buf, strlen(buf), 0);
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
    strcat(response, "]\n");
}

static int __attribute__((unused)) calculate_sound_direction(server_t *srv, player_t *sender,
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

static player_t *find_player_by_client(server_t *srv, client_t *cl) {
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->client_idx == cl->id)
            return srv->players[i];
    }
    return NULL;
}

// COMMANDES AI

static void cmd_forward(server_t *srv, client_t *cl, player_t *player) {
    (void)srv; (void)cl; // Supprimer warnings
    // Pas de vérification pending_action - on peut avoir plusieurs actions en file
    start_action(player, "forward_end", 7);
    log_info("Forward scheduled for player %d (7/f seconds)", player->id);
    // PAS de réponse immédiate - elle viendra quand l'action sera exécutée
}

static void cmd_right(server_t *srv, client_t *cl, player_t *player) {
    (void)srv; (void)cl; // Supprimer warnings
    start_action(player, "right_end", 7);
    log_info("Right scheduled for player %d (7/f seconds)", player->id);
}

static void cmd_left(server_t *srv, client_t *cl, player_t *player) {
    (void)srv; (void)cl; // Supprimer warnings
    start_action(player, "left_end", 7);
    log_info("Left scheduled for player %d (7/f seconds)", player->id);
}

// COMMANDES INSTANTANÉES
static void cmd_look(server_t *srv, client_t *cl, player_t *player) {
    char response[4096];
    generate_look_response(srv, player, response);
    send_response(cl->socket_fd, response);
    log_info("Look executed instantly for player %d", player->id);
}

static void cmd_inventory(server_t *srv, client_t *cl, player_t *player) {
    (void)srv; // Supprimer warning
    char response[512];
    snprintf(response, sizeof(response),
        "[food %d, linemate %d, deraumere %d, sibur %d, mendiane %d, phiras %d, thystame %d]\n",
        player->inventory[0], player->inventory[1], player->inventory[2],
        player->inventory[3], player->inventory[4], player->inventory[5],
        player->inventory[6]);
    send_response(cl->socket_fd, response);
    log_info("Inventory executed instantly for player %d", player->id);
}

static void cmd_connect_nbr(server_t *srv, client_t *cl, player_t *player) {
    sendf(cl->socket_fd, "%d\n", srv->slots_remaining[player->team_idx]);
    log_info("Connect_nbr executed instantly for player %d: %d slots", player->id, srv->slots_remaining[player->team_idx]);
}

// COMMANDES ASYNCHRONES
static void cmd_take(server_t *srv, client_t *cl, player_t *player, const char *object) {
    (void)srv; // Supprimer warning
    resource_t res = resource_from_name(object);
    if (res < 0) { 
        send_response(cl->socket_fd, "ko\n"); 
        return; 
    }
    
    char action[256];
    snprintf(action, sizeof(action), "take_end %d", res);
    start_action(player, action, 7);
    log_info("Take %s scheduled for player %d (7/f seconds)", object, player->id);
}

static void cmd_set(server_t *srv, client_t *cl, player_t *player, const char *object) {
    (void)srv; // Supprimer warning
    resource_t res = resource_from_name(object);
    if (res < 0) { 
        send_response(cl->socket_fd, "ko\n"); 
        return; 
    }
    
    char action[256];
    snprintf(action, sizeof(action), "set_end %d", res);
    start_action(player, action, 7);
    log_info("Set %s scheduled for player %d (7/f seconds)", object, player->id);
}

static void cmd_broadcast(server_t *srv, client_t *cl, player_t *player, const char *message) {
    (void)srv; (void)cl; // Supprimer warnings
    char action[512];
    snprintf(action, sizeof(action), "broadcast_end %s", message);
    start_action(player, action, 7);
    log_info("Broadcast scheduled for player %d: '%.50s...' (7/f seconds)", player->id, message);
}

static void cmd_fork(server_t *srv, client_t *cl, player_t *player) {
    (void)srv; (void)cl; // Supprimer warnings
    start_action(player, "fork_end", 42);
    log_info("Fork scheduled for player %d (42/f seconds)", player->id);
}

static void cmd_eject(server_t *srv, client_t *cl, player_t *player) {
    (void)srv; (void)cl; // Supprimer warnings
    start_action(player, "eject_end", 7);
    log_info("Eject scheduled for player %d (7/f seconds)", player->id);
}

static void cmd_incantation(server_t *srv, client_t *cl, player_t *player) {
    if (check_elevation_requirements(srv, player->x, player->y, player->level)) {
        start_action(player, "incantation_end", 300);
        player->is_incanting = 1;
        
        send_response(cl->socket_fd, "Elevation underway\n");
        
        char msg[512];
        snprintf(msg, sizeof(msg), "pic %d %d %d", player->x, player->y, player->level);
        
        for (int i = 0; i < srv->player_count; i++) {
            if (srv->players[i]->x == player->x && srv->players[i]->y == player->y &&
                srv->players[i]->level == player->level) {
                char player_str[32];
                snprintf(player_str, sizeof(player_str), " #%d", srv->players[i]->id);
                strcat(msg, player_str);
                srv->players[i]->is_incanting = 1;
            }
        }
        strcat(msg, "\n");
        broadcast_to_gui(srv, msg);
        
        log_info("Incantation started for player %d at (%d,%d)", player->id, player->x, player->y);
    } else {
        send_response(cl->socket_fd, "ko\n");
        
        char msg[256];
        snprintf(msg, sizeof(msg), "pie %d %d 0\n", player->x, player->y);
        broadcast_to_gui(srv, msg);
        
        log_info("Incantation failed for player %d (requirements not met)", player->id);
    }
}

// COMMANDES GUI (restent inchangées)
static void cmd_msz(server_t *srv, client_t *cl) {
    sendf(cl->socket_fd, "msz %d %d\n", srv->width, srv->height);
}

static void cmd_bct(server_t *srv, client_t *cl, int x, int y) {
    if (x < 0 || x >= srv->width || y < 0 || y >= srv->height) { 
        send_response(cl->socket_fd, "sbp\n"); 
        return; 
    }
    tile_t *tile = &srv->map[y][x];
    sendf(cl->socket_fd, "bct %d %d %d %d %d %d %d %d %d\n",
          x, y, tile->food, tile->stones[0], tile->stones[1], tile->stones[2], 
          tile->stones[3], tile->stones[4], tile->stones[5]);
}

static void cmd_mct(server_t *srv, client_t *cl) {
    for (int y = 0; y < srv->height; y++) 
        for (int x = 0; x < srv->width; x++) 
            cmd_bct(srv, cl, x, y);
}

static void cmd_tna(server_t *srv, client_t *cl) {
    for (int i = 0; i < srv->teams_count; i++) 
        sendf(cl->socket_fd, "tna %s\n", srv->team_names[i]);
}

static void cmd_ppo(server_t *srv, client_t *cl, int player_id) {
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->id == player_id) {
            player_t *p = srv->players[i];
            sendf(cl->socket_fd, "ppo #%d %d %d %d\n", p->id, p->x, p->y, p->orientation);
            return;
        }
    }
    send_response(cl->socket_fd, "sbp\n");
}

static void cmd_plv(server_t *srv, client_t *cl, int player_id) {
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->id == player_id) {
            sendf(cl->socket_fd, "plv #%d %d\n", player_id, srv->players[i]->level);
            return;
        }
    }
    send_response(cl->socket_fd, "sbp\n");
}

static void cmd_pin(server_t *srv, client_t *cl, int player_id) {
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->id == player_id) {
            player_t *p = srv->players[i];
            sendf(cl->socket_fd, "pin #%d %d %d %d %d %d %d %d %d %d\n",
                  p->id, p->x, p->y, p->inventory[0], p->inventory[1], p->inventory[2], 
                  p->inventory[3], p->inventory[4], p->inventory[5], p->inventory[6]);
            return;
        }
    }
    send_response(cl->socket_fd, "sbp\n");
}

static void cmd_sgt(server_t *srv, client_t *cl) {
    sendf(cl->socket_fd, "sgt %d\n", srv->freq);
}

static void cmd_sst(server_t *srv, client_t *cl, int freq) {
    if (freq < 2 || freq > 10000) { 
        send_response(cl->socket_fd, "sbp\n"); 
        return; 
    }
    srv->freq = freq;
    sendf(cl->socket_fd, "sst %d\n", srv->freq);
}

void dispatch_command(server_t *srv, client_t *cl, const char *line) {
    // DEBUG: Log toutes les commandes reçues
    log_info("Received command from client %d: '%s'", cl->id, line);
    
    char buf[1024];
    strncpy(buf, line, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    char *token = strtok(buf, " \t");
    if (!token) return;
    char *cmd = token;
    char *arg1 = strtok(NULL, " \t");
    char *arg2 = strtok(NULL, " \t");

    player_t *player = NULL;
    if (!cl->is_gui) {
        player = find_player_by_client(srv, cl);
        if (!player) { 
            send_response(cl->socket_fd, "ko\n"); 
            return; 
        }
    }

    if (!cl->is_gui) {
        if (strcmp(cmd, "Forward") == 0) cmd_forward(srv, cl, player);
        else if (strcmp(cmd, "Right") == 0) cmd_right(srv, cl, player);
        else if (strcmp(cmd, "Left") == 0) cmd_left(srv, cl, player);
        else if (strcmp(cmd, "Look") == 0) cmd_look(srv, cl, player);
        else if (strcmp(cmd, "Inventory") == 0) cmd_inventory(srv, cl, player);
        else if (strcmp(cmd, "Take") == 0 && arg1) cmd_take(srv, cl, player, arg1);
        else if (strcmp(cmd, "Set") == 0 && arg1) cmd_set(srv, cl, player, arg1);
        else if (strcmp(cmd, "Broadcast") == 0) {
            const char *message = line + strlen("Broadcast");
            while (*message == ' ' || *message == '\t') message++;
            cmd_broadcast(srv, cl, player, message);
        }
        else if (strcmp(cmd, "Connect_nbr") == 0) cmd_connect_nbr(srv, cl, player);
        else if (strcmp(cmd, "Fork") == 0) cmd_fork(srv, cl, player);
        else if (strcmp(cmd, "Eject") == 0) cmd_eject(srv, cl, player);
        else if (strcmp(cmd, "Incantation") == 0) cmd_incantation(srv, cl, player);
        else send_response(cl->socket_fd, "ko\n");
    } else {
        if (strcmp(cmd, "msz") == 0) cmd_msz(srv, cl);
        else if (strcmp(cmd, "bct") == 0 && arg1 && arg2) cmd_bct(srv, cl, atoi(arg1), atoi(arg2));
        else if (strcmp(cmd, "mct") == 0) cmd_mct(srv, cl);
        else if (strcmp(cmd, "tna") == 0) cmd_tna(srv, cl);
        else if (strcmp(cmd, "ppo") == 0 && arg1) cmd_ppo(srv, cl, atoi(arg1+1));
        else if (strcmp(cmd, "plv") == 0 && arg1) cmd_plv(srv, cl, atoi(arg1+1));
        else if (strcmp(cmd, "pin") == 0 && arg1) cmd_pin(srv, cl, atoi(arg1+1));
        else if (strcmp(cmd, "sgt") == 0) cmd_sgt(srv, cl);
        else if (strcmp(cmd, "sst") == 0 && arg1) cmd_sst(srv, cl, atoi(arg1));
        else send_response(cl->socket_fd, "suc\n");
    }
}