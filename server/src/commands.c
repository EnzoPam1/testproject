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
    send(fd, msg, strlen(msg), MSG_NOSIGNAL);
}

static void sendf(int fd, const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    send(fd, buf, strlen(buf), MSG_NOSIGNAL);
}

// Generate look response with proper format for AI
static void generate_look_response(server_t *srv, player_t *player, char *response, size_t max_size)
{
    int vision_size = player->level; // Vision increases with level
    int pos = 0;
    
    pos += snprintf(response + pos, max_size - pos, "[");

    // Start with current tile (index 0)
    char tile_content[512] = "";
    int content_pos = 0;
    
    // Add players on current tile
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->x == player->x && srv->players[i]->y == player->y && 
            srv->players[i]->alive && srv->players[i] != player) {
            if (content_pos > 0) {
                content_pos += snprintf(tile_content + content_pos, sizeof(tile_content) - content_pos, " ");
            }
            content_pos += snprintf(tile_content + content_pos, sizeof(tile_content) - content_pos, "player");
        }
    }

    // Add resources on current tile
    tile_t *tile = &srv->map[player->y][player->x];
    const char *res_names[] = {"food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"};
    for (int r = 0; r < 7; r++) {
        int count = (r == 0) ? tile->food : tile->stones[r - 1];
        for (int c = 0; c < count; c++) {
            if (content_pos > 0) {
                content_pos += snprintf(tile_content + content_pos, sizeof(tile_content) - content_pos, " ");
            }
            content_pos += snprintf(tile_content + content_pos, sizeof(tile_content) - content_pos, "%s", res_names[r]);
        }
    }
    
    pos += snprintf(response + pos, max_size - pos, "%s", tile_content);

    // Add surrounding tiles based on vision level
    for (int level = 1; level <= vision_size; level++) {
        int tiles_in_level = 2 * level + 1;
        
        for (int i = 0; i < tiles_in_level; i++) {
            pos += snprintf(response + pos, max_size - pos, ",");
            
            int dx = 0, dy = 0;
            int offset = i - level; // Range from -level to +level
            
            // Calculate tile position based on player orientation
            switch (player->orientation) {
                case NORTH:
                    dx = offset;
                    dy = -level;
                    break;
                case SOUTH:
                    dx = -offset;
                    dy = level;
                    break;
                case EAST:
                    dx = level;
                    dy = offset;
                    break;
                case WEST:
                    dx = -level;
                    dy = -offset;
                    break;
            }

            int real_x = (player->x + dx + srv->width) % srv->width;
            int real_y = (player->y + dy + srv->height) % srv->height;
            tile_t *target_tile = &srv->map[real_y][real_x];
            
            char level_tile_content[512] = "";
            int level_content_pos = 0;

            // Add players on this tile
            for (int j = 0; j < srv->player_count; j++) {
                if (srv->players[j]->x == real_x && srv->players[j]->y == real_y && srv->players[j]->alive) {
                    if (level_content_pos > 0) {
                        level_content_pos += snprintf(level_tile_content + level_content_pos, sizeof(level_tile_content) - level_content_pos, " ");
                    }
                    level_content_pos += snprintf(level_tile_content + level_content_pos, sizeof(level_tile_content) - level_content_pos, "player");
                }
            }

            // Add resources on this tile
            for (int r = 0; r < 7; r++) {
                int count = (r == 0) ? target_tile->food : target_tile->stones[r - 1];
                for (int c = 0; c < count; c++) {
                    if (level_content_pos > 0) {
                        level_content_pos += snprintf(level_tile_content + level_content_pos, sizeof(level_tile_content) - level_content_pos, " ");
                    }
                    level_content_pos += snprintf(level_tile_content + level_content_pos, sizeof(level_tile_content) - level_content_pos, "%s", res_names[r]);
                }
            }
            
            pos += snprintf(response + pos, max_size - pos, "%s", level_tile_content);
        }
    }
    
    snprintf(response + pos, max_size - pos, "]\n");
}

// Calculate sound direction for broadcast (simplified version)
static int calculate_sound_direction(server_t *srv, player_t *sender, player_t *receiver)
{
    int dx = sender->x - receiver->x;
    int dy = sender->y - receiver->y;

    // Handle wrapping world
    if (abs(dx) > srv->width / 2) {
        dx = dx > 0 ? dx - srv->width : dx + srv->width;
    }
    if (abs(dy) > srv->height / 2) {
        dy = dy > 0 ? dy - srv->height : dy + srv->height;
    }

    if (dx == 0 && dy == 0) {
        return 0; // Same tile
    }

    // Simple 8-direction calculation
    double angle = atan2(dy, dx) * 180 / M_PI;
    angle = fmod(angle + 360, 360);

    // Convert to direction (1-8)
    int dir = ((int)((angle + 22.5) / 45) % 8) + 1;
    
    // Adjust for player orientation
    int orientation_offset = (receiver->orientation - 1) * 2;
    dir = ((dir - 1 + orientation_offset) % 8) + 1;
    
    return dir;
}

static player_t *find_player_by_client(server_t *srv, client_t *cl) {
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->client_idx == cl->id && srv->players[i]->alive)
            return srv->players[i];
    }
    return NULL;
}

// AI Command implementations with exact format expected by Python AI
static void cmd_forward(server_t *srv, client_t *cl, player_t *player) {
    // Set action with proper timing
    player->action_end_time = time(NULL) + (7.0 / srv->freq);
    strcpy(player->pending_action, "forward");
    
    // Execute immediately for now (timing can be added later)
    player_move_forward(player, srv);
    send_response(cl->socket_fd, "ok\n");
    
    char msg[256];
    snprintf(msg, sizeof(msg), "ppo #%d %d %d %d\n",
        player->id, player->x, player->y, player->orientation);
    broadcast_to_gui(srv, msg);
}

static void cmd_right(server_t *srv, client_t *cl, player_t *player) {
    player->action_end_time = time(NULL) + (7.0 / srv->freq);
    strcpy(player->pending_action, "right");
    
    player_turn(player, 1);
    send_response(cl->socket_fd, "ok\n");
    
    char msg[256];
    snprintf(msg, sizeof(msg), "ppo #%d %d %d %d\n",
        player->id, player->x, player->y, player->orientation);
    broadcast_to_gui(srv, msg);
}

static void cmd_left(server_t *srv, client_t *cl, player_t *player) {
    player->action_end_time = time(NULL) + (7.0 / srv->freq);
    strcpy(player->pending_action, "left");
    
    player_turn(player, -1);
    send_response(cl->socket_fd, "ok\n");
    
    char msg[256];
    snprintf(msg, sizeof(msg), "ppo #%d %d %d %d\n",
        player->id, player->x, player->y, player->orientation);
    broadcast_to_gui(srv, msg);
}

static void cmd_look(server_t *srv, client_t *cl, player_t *player) {
    char response[4096];
    generate_look_response(srv, player, response, sizeof(response));
    send_response(cl->socket_fd, response);
}

static void cmd_inventory(server_t *srv, client_t *cl, player_t *player) {
    char response[512];
    // Format exactly as expected by AI: [food n, linemate n, ...]
    snprintf(response, sizeof(response),
        "[food %d, linemate %d, deraumere %d, sibur %d, mendiane %d, phiras %d, thystame %d]\n",
        player->inventory[0], player->inventory[1], player->inventory[2],
        player->inventory[3], player->inventory[4], player->inventory[5],
        player->inventory[6]);
    send_response(cl->socket_fd, response);
}

static void cmd_take(server_t *srv, client_t *cl, player_t *player, const char *object) {
    resource_t res = resource_from_name(object);
    if (res < 0) { 
        send_response(cl->socket_fd, "ko\n"); 
        return; 
    }
    
    tile_t *tile = &srv->map[player->y][player->x];
    
    if (player_take_resource(player, tile, res)) {
        send_response(cl->socket_fd, "ok\n");
        
        char msg[256];
        snprintf(msg, sizeof(msg), "pgt #%d %d\n", player->id, res);
        broadcast_to_gui(srv, msg);
        
        // Update tile content for GUI
        snprintf(msg, sizeof(msg), "bct %d %d %d %d %d %d %d %d %d\n",
            player->x, player->y, tile->food, tile->stones[0], tile->stones[1],
            tile->stones[2], tile->stones[3], tile->stones[4], tile->stones[5]);
        broadcast_to_gui(srv, msg);
        
        // Update player inventory for GUI
        snprintf(msg, sizeof(msg), "pin #%d %d %d %d %d %d %d %d %d %d\n",
            player->id, player->x, player->y, player->inventory[0], player->inventory[1], 
            player->inventory[2], player->inventory[3], player->inventory[4], player->inventory[5], player->inventory[6]);
        broadcast_to_gui(srv, msg);
    } else {
        send_response(cl->socket_fd, "ko\n");
    }
}

static void cmd_set(server_t *srv, client_t *cl, player_t *player, const char *object) {
    resource_t res = resource_from_name(object);
    if (res < 0) { 
        send_response(cl->socket_fd, "ko\n"); 
        return; 
    }
    
    tile_t *tile = &srv->map[player->y][player->x];
    
    if (player_drop_resource(player, tile, res)) {
        send_response(cl->socket_fd, "ok\n");
        
        char msg[256];
        snprintf(msg, sizeof(msg), "pdr #%d %d\n", player->id, res);
        broadcast_to_gui(srv, msg);
        
        // Update tile content for GUI
        snprintf(msg, sizeof(msg), "bct %d %d %d %d %d %d %d %d %d\n",
            player->x, player->y, tile->food, tile->stones[0], tile->stones[1],
            tile->stones[2], tile->stones[3], tile->stones[4], tile->stones[5]);
        broadcast_to_gui(srv, msg);
        
        // Update player inventory for GUI
        snprintf(msg, sizeof(msg), "pin #%d %d %d %d %d %d %d %d %d %d\n",
            player->id, player->x, player->y, player->inventory[0], player->inventory[1], 
            player->inventory[2], player->inventory[3], player->inventory[4], player->inventory[5], player->inventory[6]);
        broadcast_to_gui(srv, msg);
    } else {
        send_response(cl->socket_fd, "ko\n");
    }
}

static void cmd_broadcast(server_t *srv, client_t *cl, player_t *player, const char *message) {
    // Send message to all other players with direction
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i] == player || !srv->players[i]->alive)
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
            sendf(recv_cl->socket_fd, "message %d, %s\n", direction, message);
        }
    }
    
    send_response(cl->socket_fd, "ok\n");
    
    char msg[512];
    snprintf(msg, sizeof(msg), "pbc #%d %s\n", player->id, message);
    broadcast_to_gui(srv, msg);
}

static void cmd_connect_nbr(server_t *srv, client_t *cl, player_t *player) {
    sendf(cl->socket_fd, "%d\n", srv->slots_remaining[player->team_idx]);
}

static void cmd_fork(server_t *srv, client_t *cl, player_t *player) {
    // Add slot for team
    srv->slots_remaining[player->team_idx]++;
    send_response(cl->socket_fd, "ok\n");
    
    char msg[256];
    snprintf(msg, sizeof(msg), "pfk #%d\n", player->id);
    broadcast_to_gui(srv, msg);
    
    // Create egg for GUI
    static int egg_id = 1;
    snprintf(msg, sizeof(msg), "enw #%d #%d %d %d\n",
        egg_id++, player->id, player->x, player->y);
    broadcast_to_gui(srv, msg);
}

static void cmd_eject(server_t *srv, client_t *cl, player_t *player) {
    int ejected = 0;
    
    // Eject all players on the same tile
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i] != player &&
            srv->players[i]->x == player->x &&
            srv->players[i]->y == player->y &&
            srv->players[i]->alive) {
            
            // Move player forward in the direction the ejector is facing
            player_move_forward(srv->players[i], srv);
            
            // Notify ejected player
            client_t *ejected_cl = NULL;
            for (int j = 0; j < srv->client_count; j++) {
                if (srv->clients[j]->id == srv->players[i]->client_idx) {
                    ejected_cl = srv->clients[j];
                    break;
                }
            }
            
            if (ejected_cl) {
                int dir = calculate_sound_direction(srv, player, srv->players[i]);
                sendf(ejected_cl->socket_fd, "eject: %d\n", dir);
            }
            
            char msg[256];
            snprintf(msg, sizeof(msg), "pex #%d\n", srv->players[i]->id);
            broadcast_to_gui(srv, msg);
            
            // Update position for GUI
            snprintf(msg, sizeof(msg), "ppo #%d %d %d %d\n",
                srv->players[i]->id, srv->players[i]->x, srv->players[i]->y, srv->players[i]->orientation);
            broadcast_to_gui(srv, msg);
            
            ejected = 1;
        }
    }
    
    send_response(cl->socket_fd, ejected ? "ok\n" : "ko\n");
}

static void cmd_incantation(server_t *srv, client_t *cl, player_t *player) {
    if (check_elevation_requirements(srv, player->x, player->y, player->level)) {
        player->action_end_time = time(NULL) + (300.0 / srv->freq);
        player->is_incanting = 1;
        strcpy(player->pending_action, "incantation_end");
        
        send_response(cl->socket_fd, "Elevation underway\n");
        
        // Notify GUI of incantation start
        char msg[512];
        snprintf(msg, sizeof(msg), "pic %d %d %d", player->x, player->y, player->level);
        
        // Add all participating players
        for (int i = 0; i < srv->player_count; i++) {
            if (srv->players[i]->x == player->x && srv->players[i]->y == player->y &&
                srv->players[i]->level == player->level && srv->players[i]->alive) {
                char player_str[32];
                snprintf(player_str, sizeof(player_str), " #%d", srv->players[i]->id);
                strcat(msg, player_str);
                srv->players[i]->is_incanting = 1;
            }
        }
        strcat(msg, "\n");
        broadcast_to_gui(srv, msg);
        
        log_info("Started incantation for player #%d at (%d,%d) level %d", 
               player->id, player->x, player->y, player->level);
    } else {
        send_response(cl->socket_fd, "ko\n");
        
        char msg[256];
        snprintf(msg, sizeof(msg), "pie %d %d 0\n", player->x, player->y);
        broadcast_to_gui(srv, msg);
    }
}

// GUI command implementations
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
    for (int y = 0; y < srv->height; y++) {
        for (int x = 0; x < srv->width; x++) {
            cmd_bct(srv, cl, x, y);
        }
    }
}

static void cmd_tna(server_t *srv, client_t *cl) {
    for (int i = 0; i < srv->teams_count; i++) {
        sendf(cl->socket_fd, "tna %s\n", srv->team_names[i]);
    }
}

static void cmd_sgt(server_t *srv, client_t *cl) {
    sendf(cl->socket_fd, "sgt %d\n", srv->freq);
}

void dispatch_command(server_t *srv, client_t *cl, const char *line) {
    if (!line || strlen(line) == 0) {
        send_response(cl->socket_fd, cl->is_gui ? "suc\n" : "ko\n");
        return;
    }

    char buf[1024];
    strncpy(buf, line, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    
    char *token = strtok(buf, " \t");
    if (!token) {
        send_response(cl->socket_fd, cl->is_gui ? "suc\n" : "ko\n");
        return;
    }
    
    char *cmd = token;
    char *arg1 = strtok(NULL, " \t");
    char *arg2 = strtok(NULL, " \t");

    // Find player for AI clients
    player_t *player = NULL;
    if (!cl->is_gui) {
        player = find_player_by_client(srv, cl);
        if (!player) { 
            send_response(cl->socket_fd, "ko\n"); 
            return; 
        }
        
        // Check if player is alive
        if (!player->alive) {
            send_response(cl->socket_fd, "dead\n");
            return;
        }
    }

    // Process AI commands
    if (!cl->is_gui) {
        if (strcmp(cmd, "Forward") == 0) {
            cmd_forward(srv, cl, player);
        } else if (strcmp(cmd, "Right") == 0) {
            cmd_right(srv, cl, player);
        } else if (strcmp(cmd, "Left") == 0) {
            cmd_left(srv, cl, player);
        } else if (strcmp(cmd, "Look") == 0) {
            cmd_look(srv, cl, player);
        } else if (strcmp(cmd, "Inventory") == 0) {
            cmd_inventory(srv, cl, player);
        } else if (strcmp(cmd, "Take") == 0 && arg1) {
            cmd_take(srv, cl, player, arg1);
        } else if (strcmp(cmd, "Set") == 0 && arg1) {
            cmd_set(srv, cl, player, arg1);
        } else if (strcmp(cmd, "Broadcast") == 0) {
            const char *message = line + strlen("Broadcast");
            while (*message == ' ' || *message == '\t') message++;
            cmd_broadcast(srv, cl, player, message);
        } else if (strcmp(cmd, "Connect_nbr") == 0) {
            cmd_connect_nbr(srv, cl, player);
        } else if (strcmp(cmd, "Fork") == 0) {
            cmd_fork(srv, cl, player);
        } else if (strcmp(cmd, "Eject") == 0) {
            cmd_eject(srv, cl, player);
        } else if (strcmp(cmd, "Incantation") == 0) {
            cmd_incantation(srv, cl, player);
        } else {
            send_response(cl->socket_fd, "ko\n");
        }
    } 
    // Process GUI commands
    else {
        if (strcmp(cmd, "msz") == 0) {
            cmd_msz(srv, cl);
        } else if (strcmp(cmd, "bct") == 0 && arg1 && arg2) {
            cmd_bct(srv, cl, atoi(arg1), atoi(arg2));
        } else if (strcmp(cmd, "mct") == 0) {
            cmd_mct(srv, cl);
        } else if (strcmp(cmd, "tna") == 0) {
            cmd_tna(srv, cl);
        } else if (strcmp(cmd, "sgt") == 0) {
            cmd_sgt(srv, cl);
        } else {
            send_response(cl->socket_fd, "suc\n");
        }
    }
}