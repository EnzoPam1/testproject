/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Game logic
*/

#ifndef GAME_H_
#define GAME_H_

#include <stdbool.h>
#include "map.h"
#include "team.h"
#include "player.h"

// Forward declarations
typedef struct game_s game_t;

// Game structure
typedef struct game_s {
    map_t *map;
    team_t **teams;
    int team_count;
    player_t **players;
    int player_count;
    int player_capacity;
    
    // Game state
    int next_player_id;
    int next_egg_id;
    bool game_won;
    char *winning_team;
    
    // Resource spawning
    int resource_timer;
} game_t;

// Game functions
game_t *game_create(int width, int height, char **team_names, int team_count, int clients_nb);
void game_destroy(game_t *game);
player_t *game_add_player(game_t *game, int client_id, const char *team_name);
void game_remove_player(game_t *game, int player_id);
team_t *game_get_team_by_name(game_t *game, const char *name);
player_t *game_get_player_by_id(game_t *game, int player_id);
player_t *game_get_player_by_client_id(game_t *game, int client_id);
void game_tick(game_t *game, int freq);
bool game_check_victory(game_t *game);
void game_spawn_resources(game_t *game);

#endif /* !GAME_H_ */