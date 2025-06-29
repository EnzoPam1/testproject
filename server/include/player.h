/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Player structure and functions
*/

#pragma once

#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#define MAX_INVENTORY 7
#define LIFE_UNIT_DURATION 126

typedef enum {
    NORTH = 1,
    EAST = 2,
    SOUTH = 3,
    WEST = 4
} orientation_t;

typedef enum {
    RES_FOOD = 0,
    RES_LINEMATE,
    RES_DERAUMERE,
    RES_SIBUR,
    RES_MENDIANE,
    RES_PHIRAS,
    RES_THYSTAME
} resource_t;

// Forward declarations to avoid recursive includes
typedef struct s_server server_t;
typedef struct s_tile tile_t;

typedef struct s_player {
    int id;
    int client_idx;
    int team_idx;
    int x;
    int y;
    orientation_t orientation;
    int level;
    int life_units;
    bool alive;
    int inventory[MAX_INVENTORY];
    int is_incanting;
    time_t action_end_time;
    char pending_action[256];
} player_t;

// Core player functions
player_t *player_create(int client_idx, int team_idx, int x, int y);
void player_destroy(player_t *player);

// Life and state management
void player_consume_life(player_t *player, server_t *srv);
void player_reset_action(player_t *player);
bool player_is_busy(player_t *player);
void player_set_action(player_t *player, const char *action, int duration);

// Movement and orientation
void player_turn(player_t *player, int direction);
void player_move_forward(player_t *player, server_t *srv);

// Resource management
int player_take_resource(player_t *player, tile_t *tile, resource_t res);
int player_drop_resource(player_t *player, tile_t *tile, resource_t res);
bool player_has_resource(player_t *player, resource_t res, int amount);
bool player_has_enough_resources_for_elevation(player_t *player);

// Vision and positioning
bool player_can_see_tile(player_t *player, int tile_x, int tile_y);
int player_get_vision_tiles(player_t *player, server_t *srv, int **tiles_x, int **tiles_y);

// Multi-player functions
int player_count_at_position(server_t *srv, int x, int y, int level);
player_t **player_get_at_position(server_t *srv, int x, int y, int *count);

// Debug and utility
void player_print_status(player_t *player);