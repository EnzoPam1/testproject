/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Player management functions
*/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "player.h"
#include "server.h"
#include "game.h"
#include "utils.h"

player_t *player_create(int client_idx, int team_idx, int x, int y)
{
    player_t *player = malloc(sizeof(player_t));
    static int next_id = 1;

    if (!player)
        die("malloc player");
    player->id = next_id++;
    player->client_idx = client_idx;
    player->team_idx = team_idx;
    player->x = x;
    player->y = y;
    player->orientation = (rand() % 4) + 1;
    player->level = 1;
    player->life_units = LIFE_UNIT_DURATION;
    player->alive = true;
    memset(player->inventory, 0, sizeof(player->inventory));
    player->inventory[RES_FOOD] = 9;
    player->is_incanting = 0;
    player->action_end_time = 0;
    player->pending_action[0] = '\0';
    log_info("Player #%d created at (%d, %d), team %d",
        player->id, x, y, team_idx);
    return player;
}

void player_destroy(player_t *player)
{
    if (!player)
        return;
    free(player);
}

void player_consume_life(player_t *player, server_t *srv)
{
    if (player->alive == false)
        return;
    player->life_units--;
    if (player->life_units <= 0) {
        if (player->inventory[RES_FOOD] > 0) {
            player->inventory[RES_FOOD]--;
            player->life_units += LIFE_UNIT_DURATION;
            log_info("Player #%d consumes 1 food, %d left",
                player->id, player->inventory[RES_FOOD]);
        } else {
            player->life_units = 0;
            log_info("Player #%d dies of starvation", player->id);
            player->alive = false;
        }
    }
}

void player_turn(player_t *player, int direction)
{
    player->orientation = player->orientation + direction;
    if (player->orientation > 4)
        player->orientation = 1;
    if (player->orientation < 1)
        player->orientation = 4;
}

void player_move_forward(player_t *player, server_t *srv)
{
    int new_x = player->x;
    int new_y = player->y;

    switch (player->orientation) {
        case NORTH:
            new_y = (player->y - 1 + srv->height) % srv->height;
            break;
        case SOUTH:
            new_y = (player->y + 1) % srv->height;
            break;
        case EAST:
            new_x = (player->x + 1) % srv->width;
            break;
        case WEST:
            new_x = (player->x - 1 + srv->width) % srv->width;
            break;
    }
    player->x = new_x;
    player->y = new_y;
}

int player_take_resource(player_t *player, tile_t *tile, resource_t res)
{
    int *tile_res = (res == RES_FOOD) ? &tile->food : &tile->stones[res - 1];

    if (*tile_res > 0) {
        (*tile_res)--;
        player->inventory[res]++;
        return 1;
    }
    return 0;
}

int player_drop_resource(player_t *player, tile_t *tile, resource_t res)
{
    if (player->inventory[res] > 0) {
        player->inventory[res]--;
        if (res == RES_FOOD)
            tile->food++;
        else
            tile->stones[res - 1]++;
        return 1;
    }
    return 0;
}
