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
#define MAX_PENDING_ACTIONS 10

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

typedef struct s_pending_action {
    clock_t start_time;
    int duration;  // en cycles (7, 42, 300, etc.)
    char action[256];
} pending_action_t;

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
    
    // File d'actions en attente
    pending_action_t actions[MAX_PENDING_ACTIONS];
    int action_count;
} player_t;

player_t *player_create(int client_idx, int team_idx, int x, int y);
void player_destroy(player_t *player);
void player_consume_life(player_t *player, server_t *srv);
void player_turn(player_t *player, int direction);
void player_move_forward(player_t *player, server_t *srv);
int player_take_resource(player_t *player, tile_t *tile, resource_t res);
int player_drop_resource(player_t *player, tile_t *tile, resource_t res);

// Nouvelles fonctions pour timing précis
void add_action(player_t *player, const char *action, int duration);
pending_action_t *get_ready_action(player_t *player, int freq);
void remove_action(player_t *player, int index);