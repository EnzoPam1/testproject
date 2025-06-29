/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Game logic implementation - Improved version
*/

#include "game.h"
#include "server.h"
#include "player.h"
#include "utils.h"
#include "actions.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const float RESOURCE_DENSITY[] = {
    0.5,   // food
    0.3,   // linemate
    0.15,  // deraumere
    0.1,   // sibur
    0.1,   // mendiane
    0.08,  // phiras
    0.05   // thystame
};

static const char* RESOURCE_NAMES[] = {
    "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"
};

static void distribute_resources_evenly(tile_t **map, int width, int height, int resource_type, int total_count)
{
    if (total_count <= 0) return;
    
    // Use a more even distribution algorithm
    int total_tiles = width * height;
    int resources_placed = 0;
    
    // First pass: place one resource per "resource_density" tiles
    int interval = total_tiles / total_count;
    if (interval < 1) interval = 1;
    
    for (int i = 0; i < total_tiles && resources_placed < total_count; i += interval) {
        int x = (i % width);
        int y = (i / width) % height;
        
        if (resource_type == 0) {
            map[y][x].food++;
        } else {
            map[y][x].stones[resource_type - 1]++;
        }
        resources_placed++;
    }
    
    // Second pass: distribute remaining resources randomly
    while (resources_placed < total_count) {
        int x = rand() % width;
        int y = rand() % height;
        
        if (resource_type == 0) {
            map[y][x].food++;
        } else {
            map[y][x].stones[resource_type - 1]++;
        }
        resources_placed++;
    }
}

static void distribute_resources(tile_t **map, int width, int height)
{
    int total_tiles = width * height;

    for (int res = 0; res < 7; res++) {
        int count = (int)(total_tiles * RESOURCE_DENSITY[res]);
        if (count < 1) count = 1;

        log_info("Distributing %d %s across the map", count, RESOURCE_NAMES[res]);
        distribute_resources_evenly(map, width, height, res, count);
    }
}

tile_t **game_map_init(int width, int height, float density[])
{
    (void)density; // unused parameter
    
    tile_t **map = malloc(sizeof(tile_t*) * height);
    if (!map) {
        die("malloc map pointers");
    }

    for (int y = 0; y < height; y++) {
        map[y] = malloc(sizeof(tile_t) * width);
        if (!map[y]) {
            die("malloc map row");
        }
        memset(map[y], 0, sizeof(tile_t) * width);
    }
    
    distribute_resources(map, width, height);
    log_info("Game map initialized (%dx%d)", width, height);
    return map;
}

void game_map_free(tile_t **map, int height)
{
    if (!map) return;
    
    for (int y = 0; y < height; y++) {
        free(map[y]);
    }
    free(map);
}

static void respawn_resources(server_t *srv)
{
    static int last_respawn_tick = 0;
    int respawn_interval = 20 * srv->freq; // 20 time units
    
    if (srv->tick_count - last_respawn_tick >= respawn_interval) {
        log_info("Respawning resources (tick %d)", srv->tick_count);
        distribute_resources(srv->map, srv->width, srv->height);
        last_respawn_tick = srv->tick_count;
        
        // Notify GUI of map changes
        char msg[256];
        for (int y = 0; y < srv->height; y++) {
            for (int x = 0; x < srv->width; x++) {
                tile_t *tile = &srv->map[y][x];
                snprintf(msg, sizeof(msg), "bct %d %d %d %d %d %d %d %d %d\n",
                    x, y, tile->food, tile->stones[0], tile->stones[1],
                    tile->stones[2], tile->stones[3], tile->stones[4], tile->stones[5]);
                broadcast_to_gui(srv, msg);
            }
        }
    }
}

int check_elevation_requirements(server_t *srv, int x, int y, int level)
{
    if (level < 1 || level > 7) return 0;
    
    tile_t *tile = &srv->map[y][x];
    
    // Elevation requirements table [level][resource_index]
    // resource_index: 0=players, 1=linemate, 2=deraumere, 3=sibur, 4=mendiane, 5=phiras, 6=thystame
    static const int requirements[8][7] = {
        {0, 0, 0, 0, 0, 0, 0}, // level 0 (unused)
        {1, 1, 0, 0, 0, 0, 0}, // level 1->2
        {2, 1, 1, 1, 0, 0, 0}, // level 2->3
        {2, 2, 0, 1, 0, 2, 0}, // level 3->4
        {4, 1, 1, 2, 0, 1, 0}, // level 4->5
        {4, 1, 2, 1, 3, 0, 0}, // level 5->6
        {6, 1, 2, 3, 0, 1, 0}, // level 6->7
        {6, 2, 2, 2, 2, 2, 1}  // level 7->8
    };

    // Count players of the same level at this position
    int player_count = 0;
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->x == x && srv->players[i]->y == y &&
            srv->players[i]->level == level && srv->players[i]->alive) {
            player_count++;
        }
    }

    // Check player requirement
    if (player_count < requirements[level][0]) {
        return 0;
    }

    // Check resource requirements
    for (int i = 1; i < 7; i++) {
        int available = tile->stones[i - 1];
        int needed = requirements[level][i];
        if (available < needed) {
            return 0;
        }
    }

    return 1;
}

void perform_elevation(server_t *srv, int x, int y, int level)
{
    if (level < 1 || level > 7) return;
    
    tile_t *tile = &srv->map[y][x];
    
    // Resource requirements for elevation
    static const int requirements[8][6] = {
        {0, 0, 0, 0, 0, 0}, // level 0 (unused)
        {1, 0, 0, 0, 0, 0}, // level 1->2
        {1, 1, 1, 0, 0, 0}, // level 2->3
        {2, 0, 1, 0, 2, 0}, // level 3->4
        {1, 1, 2, 0, 1, 0}, // level 4->5
        {1, 2, 1, 3, 0, 0}, // level 5->6
        {1, 2, 3, 0, 1, 0}, // level 6->7
        {2, 2, 2, 2, 2, 1}  // level 7->8
    };

    // Consume resources
    for (int i = 0; i < 6; i++) {
        tile->stones[i] -= requirements[level][i];
        if (tile->stones[i] < 0) tile->stones[i] = 0;
    }

    // Elevate all eligible players
    int elevated_count = 0;
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->x == x && srv->players[i]->y == y &&
            srv->players[i]->level == level && srv->players[i]->alive) {
            srv->players[i]->level++;
            srv->players[i]->is_incanting = 0;
            elevated_count++;
            
            log_info("Player #%d elevated to level %d", 
                srv->players[i]->id, srv->players[i]->level);
            
            // Notify GUI of level change
            char msg[256];
            snprintf(msg, sizeof(msg), "plv #%d %d\n", 
                srv->players[i]->id, srv->players[i]->level);
            broadcast_to_gui(srv, msg);
        }
    }

    // Notify GUI of successful elevation
    char msg[256];
    snprintf(msg, sizeof(msg), "pie %d %d 1\n", x, y);
    broadcast_to_gui(srv, msg);
    
    // Update tile content for GUI
    snprintf(msg, sizeof(msg), "bct %d %d %d %d %d %d %d %d %d\n",
        x, y, tile->food, tile->stones[0], tile->stones[1],
        tile->stones[2], tile->stones[3], tile->stones[4], tile->stones[5]);
    broadcast_to_gui(srv, msg);

    log_info("Elevation completed at (%d,%d): %d players elevated from level %d to %d", 
        x, y, elevated_count, level, level + 1);

    // Check for victory condition
    int level8_count = 0;
    char *winning_team = NULL;
    
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->level >= 8 && srv->players[i]->alive) {
            level8_count++;
            if (!winning_team) {
                winning_team = srv->team_names[srv->players[i]->team_idx];
            }
        }
    }

    if (level8_count >= 6 && winning_team) {
        log_info("VICTORY! Team '%s' wins with %d players at level 8!", 
            winning_team, level8_count);
        
        char victory_msg[256];
        snprintf(victory_msg, sizeof(victory_msg), "seg %s\n", winning_team);
        broadcast_to_gui(srv, victory_msg);
    }
}

static void check_and_process_incantations(server_t *srv)
{
    // Look for tiles with incanting players
    for (int y = 0; y < srv->height; y++) {
        for (int x = 0; x < srv->width; x++) {
            player_t *incanting_player = NULL;
            
            // Find if there's an incanting player on this tile
            for (int i = 0; i < srv->player_count; i++) {
                if (srv->players[i]->x == x && srv->players[i]->y == y &&
                    srv->players[i]->is_incanting && srv->players[i]->alive) {
                    incanting_player = srv->players[i];
                    break;
                }
            }
            
            if (incanting_player) {
                // Check if requirements are still met
                if (check_elevation_requirements(srv, x, y, incanting_player->level)) {
                    // Create GUI message for incantation in progress
                    char msg[512];
                    snprintf(msg, sizeof(msg), "pic %d %d %d", x, y, incanting_player->level);
                    
                    // Add all participating players
                    for (int i = 0; i < srv->player_count; i++) {
                        if (srv->players[i]->x == x && srv->players[i]->y == y &&
                            srv->players[i]->level == incanting_player->level && 
                            srv->players[i]->alive) {
                            char player_str[32];
                            snprintf(player_str, sizeof(player_str), " #%d", srv->players[i]->id);
                            strcat(msg, player_str);
                            
                            // Ensure all eligible players are marked as incanting
                            if (!srv->players[i]->is_incanting) {
                                srv->players[i]->is_incanting = 1;
                                
                                // Notify player of elevation start
                                for (int j = 0; j < srv->client_count; j++) {
                                    if (srv->clients[j]->id == srv->players[i]->client_idx) {
                                        send(srv->clients[j]->socket_fd, "Elevation underway\n", 20, 0);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    strcat(msg, "\n");
                    broadcast_to_gui(srv, msg);
                } else {
                    // Requirements no longer met, cancel incantation
                    log_info("Incantation cancelled at (%d,%d) - requirements not met", x, y);
                    
                    for (int i = 0; i < srv->player_count; i++) {
                        if (srv->players[i]->x == x && srv->players[i]->y == y &&
                            srv->players[i]->is_incanting && srv->players[i]->alive) {
                            srv->players[i]->is_incanting = 0;
                            srv->players[i]->pending_action[0] = '\0';
                            srv->players[i]->action_end_time = 0;
                            
                            // Notify player of failure
                            for (int j = 0; j < srv->client_count; j++) {
                                if (srv->clients[j]->id == srv->players[i]->client_idx) {
                                    send(srv->clients[j]->socket_fd, "ko\n", 3, 0);
                                    break;
                                }
                            }
                        }
                    }
                    
                    // Notify GUI of failed incantation
                    char msg[256];
                    snprintf(msg, sizeof(msg), "pie %d %d 0\n", x, y);
                    broadcast_to_gui(srv, msg);
                }
            }
        }
    }
}

void game_tick(server_t *srv)
{
    srv->tick_count++;
    
    // Handle player life consumption
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->alive) {
            player_consume_life(srv->players[i], srv);
        }
    }

    // Execute pending timed actions
    execute_pending_actions(srv);
    
    // Check ongoing incantations
    check_and_process_incantations(srv);
    
    // Respawn resources periodically
    respawn_resources(srv);
    
    // Cleanup dead players periodically (every 100 ticks)
    if (srv->tick_count % 100 == 0) {
        int alive_count = 0;
        for (int i = 0; i < srv->player_count; i++) {
            if (srv->players[i]->alive) {
                alive_count++;
            }
        }
        log_info("Game tick %d: %d players alive", srv->tick_count, alive_count);
    }
}

int game_get_total_resource_count(server_t *srv, int resource_type)
{
    int total = 0;
    
    for (int y = 0; y < srv->height; y++) {
        for (int x = 0; x < srv->width; x++) {
            if (resource_type == 0) {
                total += srv->map[y][x].food;
            } else if (resource_type >= 1 && resource_type <= 6) {
                total += srv->map[y][x].stones[resource_type - 1];
            }
        }
    }
    
    return total;
}

void game_print_statistics(server_t *srv)
{
    log_info("=== Game Statistics ===");
    log_info("Tick: %d", srv->tick_count);
    log_info("Players: %d alive", srv->player_count);
    
    for (int i = 0; i < 7; i++) {
        int total = game_get_total_resource_count(srv, i);
        log_info("Total %s: %d", RESOURCE_NAMES[i], total);
    }
    
    // Team statistics
    for (int team = 0; team < srv->teams_count; team++) {
        int team_players = 0;
        int max_level = 0;
        
        for (int i = 0; i < srv->player_count; i++) {
            if (srv->players[i]->team_idx == team && srv->players[i]->alive) {
                team_players++;
                if (srv->players[i]->level > max_level) {
                    max_level = srv->players[i]->level;
                }
            }
        }
        
        log_info("Team %s: %d players, max level %d, %d slots available", 
            srv->team_names[team], team_players, max_level, srv->slots_remaining[team]);
    }
    log_info("=====================");
}