/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Map management
*/

#ifndef MAP_H_
#define MAP_H_

#include "resources.h"

// Tile structure
typedef struct tile_s {
    int resources[RESOURCE_COUNT];
    int *players;      // Array of player IDs
    int player_count;
    int *eggs;         // Array of egg IDs
    int egg_count;
} tile_t;

// Map structure
typedef struct map_s {
    int width;
    int height;
    tile_t **tiles;
} map_t;

// Map functions
map_t *map_create(int width, int height);
void map_destroy(map_t *map);
tile_t *map_get_tile(map_t *map, int x, int y);
void map_add_player(map_t *map, int x, int y, int player_id);
void map_remove_player(map_t *map, int x, int y, int player_id);
void map_add_egg(map_t *map, int x, int y, int egg_id);
void map_remove_egg(map_t *map, int x, int y, int egg_id);
int map_wrap_x(map_t *map, int x);
int map_wrap_y(map_t *map, int y);

#endif /* !MAP_H_ */