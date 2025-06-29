/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Player management functions - Complete version
*/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "player.h"
#include "server.h"
#include "game.h"
#include "utils.h"
#include "actions.h"

player_t *player_create(int client_idx, int team_idx, int x, int y)
{
    player_t *player = malloc(sizeof(player_t));
    static int next_id = 1;

    if (!player)
        die("malloc player");
    
    memset(player, 0, sizeof(player_t));
    
    player->id = next_id++;
    player->client_idx = client_idx;
    player->team_idx = team_idx;
    player->x = x;
    player->y = y;
    player->orientation = (rand() % 4) + 1; // Random orientation 1-4 (N,E,S,W)
    player->level = 1;
    player->life_units = LIFE_UNIT_DURATION;
    player->alive = true;
    
    // Initialize inventory
    memset(player->inventory, 0, sizeof(player->inventory));
    player->inventory[RES_FOOD] = 10; // Start with 10 food units as per specification
    
    player->is_incanting = 0;
    player->action_end_time = 0;
    player->pending_action[0] = '\0';
    
    log_info("Player #%d created at (%d, %d), team %d, orientation %d",
        player->id, x, y, team_idx, player->orientation);
    return player;
}

void player_destroy(player_t *player)
{
    if (!player)
        return;
    free(player);
}

void player_consume_life(player_t *player, server_t *srv)
{
    if (player->alive == false)
        return;
        
    player->life_units--;
    
    if (player->life_units <= 0) {
        if (player->inventory[RES_FOOD] > 0) {
            player->inventory[RES_FOOD]--;
            player->life_units += LIFE_UNIT_DURATION;
            log_info("Player #%d consumes 1 food, %d left",
                player->id, player->inventory[RES_FOOD]);
                
            // Notify GUI of inventory change
            char msg[256];
            snprintf(msg, sizeof(msg), "pin #%d %d %d %d %d %d %d %d %d\n",
                player->id, player->x, player->y, player->inventory[0], 
                player->inventory[1], player->inventory[2], player->inventory[3], 
                player->inventory[4], player->inventory[5], player->inventory[6]);
            broadcast_to_gui(srv, msg);
        } else {
            player->life_units = 0;
            log_info("Player #%d dies of starvation", player->id);
            player->alive = false;
        }
    }
}

void player_turn(player_t *player, int direction)
{
    // direction: 1 for right, -1 for left
    if (direction > 0) {
        // Turn right
        player->orientation++;
        if (player->orientation > WEST)
            player->orientation = NORTH;
    } else {
        // Turn left  
        player->orientation--;
        if (player->orientation < NORTH)
            player->orientation = WEST;
    }
    
    log_info("Player #%d turned, new orientation: %d", player->id, player->orientation);
}

void player_move_forward(player_t *player, server_t *srv)
{
    int new_x = player->x;
    int new_y = player->y;

    // Calculate new position based on orientation
    switch (player->orientation) {
        case NORTH:
            new_y = (player->y - 1 + srv->height) % srv->height;
            break;
        case SOUTH:
            new_y = (player->y + 1) % srv->height;
            break;
        case EAST:
            new_x = (player->x + 1) % srv->width;
            break;
        case WEST:
            new_x = (player->x - 1 + srv->width) % srv->width;
            break;
    }
    
    log_info("Player #%d moved from (%d,%d) to (%d,%d)", 
        player->id, player->x, player->y, new_x, new_y);
    
    player->x = new_x;
    player->y = new_y;
}

int player_take_resource(player_t *player, tile_t *tile, resource_t res)
{
    if (res < 0 || res >= MAX_INVENTORY) return 0;
    
    int *tile_res = (res == RES_FOOD) ? &tile->food : &tile->stones[res - 1];

    if (*tile_res > 0) {
        (*tile_res)--;
        player->inventory[res]++;
        
        log_info("Player #%d took %s, now has %d", 
            player->id, 
            (res == RES_FOOD) ? "food" : 
            (res == RES_LINEMATE) ? "linemate" :
            (res == RES_DERAUMERE) ? "deraumere" :
            (res == RES_SIBUR) ? "sibur" :
            (res == RES_MENDIANE) ? "mendiane" :
            (res == RES_PHIRAS) ? "phiras" : "thystame",
            player->inventory[res]);
        
        return 1;
    }
    return 0;
}

int player_drop_resource(player_t *player, tile_t *tile, resource_t res)
{
    if (res < 0 || res >= MAX_INVENTORY) return 0;
    
    if (player->inventory[res] > 0) {
        player->inventory[res]--;
        
        if (res == RES_FOOD) {
            tile->food++;
        } else {
            tile->stones[res - 1]++;
        }
        
        log_info("Player #%d dropped %s, now has %d", 
            player->id,
            (res == RES_FOOD) ? "food" : 
            (res == RES_LINEMATE) ? "linemate" :
            (res == RES_DERAUMERE) ? "deraumere" :
            (res == RES_SIBUR) ? "sibur" :
            (res == RES_MENDIANE) ? "mendiane" :
            (res == RES_PHIRAS) ? "phiras" : "thystame",
            player->inventory[res]);
        
        return 1;
    }
    return 0;
}

bool player_can_see_tile(player_t *player, int tile_x, int tile_y)
{
    // Vision calculation based on player level and orientation
    int dx = tile_x - player->x;
    int dy = tile_y - player->y;
    int vision_range = player->level;
    
    // Basic vision range check
    if (abs(dx) > vision_range || abs(dy) > vision_range) {
        return false;
    }
    
    return true;
}

void player_reset_action(player_t *player)
{
    player->action_end_time = 0;
    player->pending_action[0] = '\0';
}

bool player_is_busy(player_t *player)
{
    time_t now = time(NULL);
    return (player->action_end_time > now) || player->is_incanting;
}

void player_set_action(player_t *player, const char *action, int duration)
{
    time_t now = time(NULL);
    player->action_end_time = now + duration;
    strncpy(player->pending_action, action, sizeof(player->pending_action) - 1);
    player->pending_action[sizeof(player->pending_action) - 1] = '\0';
}

int player_count_at_position(server_t *srv, int x, int y, int level)
{
    int count = 0;
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->x == x && srv->players[i]->y == y &&
            srv->players[i]->level == level && srv->players[i]->alive) {
            count++;
        }
    }
    return count;
}

player_t **player_get_at_position(server_t *srv, int x, int y, int *count)
{
    player_t **players = malloc(sizeof(player_t*) * srv->player_count);
    *count = 0;
    
    if (!players) return NULL;
    
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->x == x && srv->players[i]->y == y && 
            srv->players[i]->alive) {
            players[(*count)++] = srv->players[i];
        }
    }
    
    return players;
}

int player_get_vision_tiles(player_t *player, server_t *srv, int **tiles_x, int **tiles_y)
{
    int vision_range = player->level;
    int max_tiles = (2 * vision_range + 1) * (2 * vision_range + 1);
    
    *tiles_x = malloc(sizeof(int) * max_tiles);
    *tiles_y = malloc(sizeof(int) * max_tiles);
    
    if (!*tiles_x || !*tiles_y) {
        free(*tiles_x);
        free(*tiles_y);
        return 0;
    }
    
    int count = 0;
    
    // Generate vision tiles based on level and orientation
    for (int row = 0; row <= vision_range; row++) {
        int tiles_in_row = 2 * row + 1;
        int start_offset = -row;
        
        for (int col = 0; col < tiles_in_row; col++) {
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
            
            (*tiles_x)[count] = real_x;
            (*tiles_y)[count] = real_y;
            count++;
        }
    }
    
    return count;
}

bool player_has_resource(player_t *player, resource_t res, int amount)
{
    if (res < 0 || res >= MAX_INVENTORY) return false;
    return player->inventory[res] >= amount;
}

bool player_has_enough_resources_for_elevation(player_t *player)
{
    // Elevation requirements table [level-1][resource_index]
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
    
    if (player->level < 1 || player->level > 7) return false;
    
    // Check resource requirements (skip player count which is index 0)
    for (int i = 1; i < 7; i++) {
        if (player->inventory[i] < requirements[player->level][i]) {
            return false;
        }
    }
    
    return true;
}

void player_print_status(player_t *player)
{
    if (!player) return;
    
    log_info("=== Player #%d Status ===", player->id);
    log_info("Position: (%d, %d)", player->x, player->y);
    log_info("Orientation: %d", player->orientation);
    log_info("Level: %d", player->level);
    log_info("Life units: %d", player->life_units);
    log_info("Alive: %s", player->alive ? "yes" : "no");
    log_info("Incanting: %s", player->is_incanting ? "yes" : "no");
    log_info("Inventory: food=%d, linemate=%d, deraumere=%d, sibur=%d, mendiane=%d, phiras=%d, thystame=%d",
        player->inventory[0], player->inventory[1], player->inventory[2],
        player->inventory[3], player->inventory[4], player->inventory[5], player->inventory[6]);
    log_info("=========================");
}