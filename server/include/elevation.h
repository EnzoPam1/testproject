/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Elevation system
*/

#ifndef ELEVATION_H_
#define ELEVATION_H_

#include <stdbool.h>

// Forward declarations
typedef struct game_s game_t;
typedef struct player_s player_t;
typedef struct tile_s tile_t;

// Elevation requirements
typedef struct elevation_req_s {
    int players;
    int resources[6];  // linemate to thystame
} elevation_req_t;

// Elevation functions
bool elevation_check_requirements(game_t *game, player_t *player, tile_t *tile);
void elevation_start(game_t *game, player_t *initiator, int x, int y);
void elevation_complete(game_t *game, player_t *player);
const elevation_req_t *elevation_get_requirements(int level);

#endif /* !ELEVATION_H_ */