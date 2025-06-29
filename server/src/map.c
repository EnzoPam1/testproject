/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Map implementation
*/

#include <stdlib.h>
#include <string.h>
#include "map.h"

map_t *map_create(int width, int height)
{
    map_t *map = calloc(1, sizeof(map_t));
    if (!map) return NULL;

    map->width = width;
    map->height = height;
    
    // Allocate tiles
    map->tiles = calloc(height, sizeof(tile_t *));
    if (!map->tiles) {
        free(map);
        return NULL;
    }
    
    for (int y = 0; y < height; y++) {
        map->tiles[y] = calloc(width, sizeof(tile_t));
        if (!map->tiles[y]) {
            map_destroy(map);
            return NULL;
        }
    }
    
    return map;
}

void map_destroy(map_t *map)
{
    if (!map) return;
    
    if (map->tiles) {
        for (int y = 0; y < map->height; y++) {
            if (map->tiles[y]) {
                for (int x = 0; x < map->width; x++) {
                    tile_t *tile = &map->tiles[y][x];
                    if (tile->players) free(tile->players);
                    if (tile->eggs) free(tile->eggs);
                }
                free(map->tiles[y]);
            }
        }
        free(map->tiles);
    }
    
    free(map);
}

tile_t *map_get_tile(map_t *map, int x, int y)
{
    if (x < 0 || x >= map->width || y < 0 || y >= map->height) {
        return NULL;
    }
    return &map->tiles[y][x];
}

void map_add_player(map_t *map, int x, int y, int player_id)
{
    tile_t *tile = map_get_tile(map, x, y);
    if (!tile) return;
    
    tile->players = realloc(tile->players, (tile->player_count + 1) * sizeof(int));
    tile->players[tile->player_count++] = player_id;
}

void map_remove_player(map_t *map, int x, int y, int player_id)
{
    tile_t *tile = map_get_tile(map, x, y);
    if (!tile) return;
    
    // Find and remove player
    for (int i = 0; i < tile->player_count; i++) {
        if (tile->players[i] == player_id) {
            // Shift remaining players
            for (int j = i; j < tile->player_count - 1; j++) {
                tile->players[j] = tile->players[j + 1];
            }
            tile->player_count--;
            
            // Resize array
            if (tile->player_count == 0) {
                free(tile->players);
                tile->players = NULL;
            }
            break;
        }
    }
}

void map_add_egg(map_t *map, int x, int y, int egg_id)
{
    tile_t *tile = map_get_tile(map, x, y);
    if (!tile) return;
    
    tile->eggs = realloc(tile->eggs, (tile->egg_count + 1) * sizeof(int));
    tile->eggs[tile->egg_count++] = egg_id;
}

void map_remove_egg(map_t *map, int x, int y, int egg_id)
{
    tile_t *tile = map_get_tile(map, x, y);
    if (!tile) return;
    
    // Find and remove egg
    for (int i = 0; i < tile->egg_count; i++) {
        if (tile->eggs[i] == egg_id) {
            // Shift remaining eggs
            for (int j = i; j < tile->egg_count - 1; j++) {
                tile->eggs[j] = tile->eggs[j + 1];
            }
            tile->egg_count--;
            
            // Resize array
            if (tile->egg_count == 0) {
                free(tile->eggs);
                tile->eggs = NULL;
            }
            break;
        }
    }
}

int map_wrap_x(map_t *map, int x)
{
    x = x % map->width;
    if (x < 0) x += map->width;
    return x;
}

int map_wrap_y(map_t *map, int y)
{
    y = y % map->height;
    if (y < 0) y += map->height;
    return y;
}