/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Broadcast implementation
*/

#include <math.h>
#include "broadcast.h"
#include "game.h"
#include "player.h"
#include "network.h"
#include "server.h"

int broadcast_get_direction(player_t *sender, player_t *receiver, 
                           int map_width, int map_height)
{
    // Same position
    if (sender->x == receiver->x && sender->y == receiver->y) {
        return 0;
    }
    
    // Calculate shortest path considering wrapping
    int dx = sender->x - receiver->x;
    int dy = sender->y - receiver->y;
    
    // Adjust for wrapping
    if (abs(dx) > map_width / 2) {
        dx = dx > 0 ? dx - map_width : dx + map_width;
    }
    if (abs(dy) > map_height / 2) {
        dy = dy > 0 ? dy - map_height : dy + map_height;
    }
    
    // Calculate angle from receiver to sender
    double angle = atan2(dy, dx) * 180.0 / M_PI;
    angle = fmod(angle + 360.0, 360.0);
    
    // Convert to octant (0-7)
    int octant = (int)((angle + 22.5) / 45.0) % 8;
    
    // Adjust for receiver orientation
    int orientation_offset = 0;
    switch (receiver->orientation) {
        case NORTH: orientation_offset = 0; break;
        case EAST: orientation_offset = 2; break;
        case SOUTH: orientation_offset = 4; break;
        case WEST: orientation_offset = 6; break;
    }
    
    // Map to tiles 1-8 based on receiver's orientation
    int direction = (octant - orientation_offset + 8) % 8;
    return direction + 1;  // 1-8 instead of 0-7
}

void broadcast_send_to_all(game_t *game, player_t *sender, const char *message)
{
    extern server_t *g_server;  // Access global server
    
    for (int i = 0; i < game->player_count; i++) {
        player_t *receiver = game->players[i];
        
        // Don't send to self
        if (receiver->id == sender->id) continue;
        
        // Calculate direction
        int direction = broadcast_get_direction(sender, receiver, 
                                              game->map->width, 
                                              game->map->height);
        
        // Find receiver's client
        for (int j = 0; j < g_server->network->client_count; j++) {
            if (g_server->network->clients[j]->player_id == receiver->id) {
                network_send(g_server->network->clients[j], 
                           "message %d,%s\n", direction, message);
                break;
            }
        }
    }
}