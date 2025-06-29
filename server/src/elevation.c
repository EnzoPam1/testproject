/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Elevation implementation
*/

#include <stdlib.h>
#include "elevation.h"
#include "game.h"
#include "player.h"
#include "map.h"
#include "network.h"
#include "server.h"
#include "gui_protocol.h"

// Elevation requirements by level (1->2, 2->3, etc.)
static const elevation_req_t requirements[7] = {
    {1, {1, 0, 0, 0, 0, 0}},  // Level 1->2
    {2, {1, 1, 1, 0, 0, 0}},  // Level 2->3
    {2, {2, 0, 1, 0, 2, 0}},  // Level 3->4
    {4, {1, 1, 2, 0, 1, 0}},  // Level 4->5
    {4, {1, 2, 1, 3, 0, 0}},  // Level 5->6
    {6, {1, 2, 3, 0, 1, 0}},  // Level 6->7
    {6, {2, 2, 2, 2, 2, 1}}   // Level 7->8
};

const elevation_req_t *elevation_get_requirements(int level)
{
    if (level < 1 || level > 7) return NULL;
    return &requirements[level - 1];
}

bool elevation_check_requirements(game_t *game, player_t *player, tile_t *tile)
{
    if (player->level < 1 || player->level > 7) return false;
    
    const elevation_req_t *req = &requirements[player->level - 1];
    
    // Check player count at same level
    int player_count = 0;
    for (int i = 0; i < tile->player_count; i++) {
        player_t *p = game_get_player_by_id(game, tile->players[i]);
        if (p && p->level == player->level && !p->is_incanting) {
            player_count++;
        }
    }
    
    if (player_count < req->players) return false;
    
    // Check resources (skip food at index 0)
    for (int i = 0; i < 6; i++) {
        if (tile->resources[i + 1] < req->resources[i]) {
            return false;
        }
    }
    
    return true;
}

void elevation_start(game_t *game, player_t *initiator, int x, int y)
{
    extern server_t *g_server;
    tile_t *tile = map_get_tile(game->map, x, y);
    const elevation_req_t *req = &requirements[initiator->level - 1];
    
    // Collect participating players
    int *participants = malloc(req->players * sizeof(int));
    int count = 0;
    
    for (int i = 0; i < tile->player_count && count < req->players; i++) {
        player_t *p = game_get_player_by_id(game, tile->players[i]);
        if (p && p->level == initiator->level && !p->is_incanting) {
            p->is_incanting = true;
            p->action.duration = DURATION_INCANTATION;
            participants[count++] = p->id;
            
            // Send message to other participants
            if (p->id != initiator->id) {
                for (int j = 0; j < g_server->network->client_count; j++) {
                    if (g_server->network->clients[j]->player_id == p->id) {
                        network_send(g_server->network->clients[j], 
                                   "Elevation underway\n");
                        break;
                    }
                }
            }
        }
    }
    
    // Consume resources
    for (int i = 0; i < 6; i++) {
        tile->resources[i + 1] -= req->resources[i];
    }
    
    // Notify GUI
    gui_notify_incantation_start(g_server, x, y, initiator->level, 
                                participants, count);
    gui_notify_tile_content(g_server, x, y);
    
    free(participants);
}

void elevation_complete(game_t *game, player_t *player)
{
    extern server_t *g_server;
    tile_t *tile = map_get_tile(game->map, player->x, player->y);
    
    // Find all participants at this location and level
    int success = 1;
    for (int i = 0; i < tile->player_count; i++) {
        player_t *p = game_get_player_by_id(game, tile->players[i]);
        if (p && p->is_incanting && p->level == player->level) {
            p->is_incanting = false;
            p->level++;
            
            // Send result to player
            for (int j = 0; j < g_server->network->client_count; j++) {
                if (g_server->network->clients[j]->player_id == p->id) {
                    network_send(g_server->network->clients[j], 
                               "Current level: %d\n", p->level);
                    break;
                }
            }
            
            // Notify GUI
            gui_notify_player_level(g_server, p);
        }
    }
    
    // Notify GUI of incantation end
    gui_notify_incantation_end(g_server, player->x, player->y, success);
}