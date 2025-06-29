/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Player structure and management
*/

#ifndef PLAYER_H_
#define PLAYER_H_

#include <stdbool.h>
#include <time.h>
#include "resources.h"

// Orientations
typedef enum {
    NORTH = 1,
    EAST = 2,
    SOUTH = 3,
    WEST = 4
} orientation_t;

// Player structure
typedef struct player_s {
    int id;
    int client_id;  // Associated client
    int team_id;
    
    // Position and orientation
    int x;
    int y;
    orientation_t orientation;
    
    // Stats
    int level;
    int life_units;  // 126 units = 1 food
    
    // Inventory
    int inventory[RESOURCE_COUNT];
    
    // State
    bool is_incanting;
    bool is_dead;
    
    // Current action
    struct {
        char *command;
        time_t end_time;
        int duration;  // in time units
    } action;
    
} player_t;

// Player functions
player_t *player_create(int id, int client_id, int team_id, int x, int y);
void player_destroy(player_t *player);
void player_move_forward(player_t *player, int map_width, int map_height);
void player_turn_right(player_t *player);
void player_turn_left(player_t *player);
void player_consume_life(player_t *player);
bool player_is_alive(player_t *player);
void player_set_action(player_t *player, const char *command, int duration);
bool player_action_done(player_t *player, int freq);

#endif /* !PLAYER_H_ */