/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Player management functions - Fixed for AI compatibility
*/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "player.h"
#include "server.h"
#include "game.h"
#include "utils.h"

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
            snprintf(msg, sizeof(msg), "pin #%d %d %d %d %d %d %d %d %d %d\n",
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
    // Simple vision calculation - can be improved
    int dx = abs(tile_x - player->x);
    int dy = abs(tile_y - player->y);
    int vision_range = player->level;
    
    return (dx <= vision_range && dy <= vision_range);
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