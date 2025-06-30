/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Player implementation
*/

#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "utils.h"

player_t *player_create(int id, int client_id, int team_id, int x, int y)
{
    player_t *player = calloc(1, sizeof(player_t));
    if (!player) return NULL;

    player->id = id;
    player->client_id = client_id;
    player->team_id = team_id;
    player->x = x;
    player->y = y;
    player->orientation = (rand() % 4) + 1;  // Random orientation
    player->level = 1;
    player->life_units = 1260;  // 10 food * 126 units
    
    // Start with 10 food
    player->inventory[RES_FOOD] = 10;

    return player;
}

void player_destroy(player_t *player)
{
    if (!player) return;
    free(player);
}

void player_move_forward(player_t *player, int map_width, int map_height)
{
    switch (player->orientation) {
        case NORTH:
            player->y = (player->y - 1 + map_height) % map_height;
            break;
        case EAST:
            player->x = (player->x + 1) % map_width;
            break;
        case SOUTH:
            player->y = (player->y + 1) % map_height;
            break;
        case WEST:
            player->x = (player->x - 1 + map_width) % map_width;
            break;
    }
}

void player_turn_right(player_t *player)
{
    player->orientation = (player->orientation % 4) + 1;
}

void player_turn_left(player_t *player)
{
    player->orientation = ((player->orientation - 2 + 4) % 4) + 1;
}

void player_consume_life(player_t *player)
{
    player->life_units--;
    
    // Consume food when life runs out
    if (player->life_units <= 0 && player->inventory[RES_FOOD] > 0) {
        player->inventory[RES_FOOD]--;
        player->life_units += 126;
    } else if (player->life_units <= 0) {
        player->is_dead = true;
    }
}

bool player_is_alive(player_t *player)
{
    return !player->is_dead;
}