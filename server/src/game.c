/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Game logic implementation
*/

#include "game.h"
#include "server.h"
#include "player.h"
#include "utils.h"
#include "actions.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const float RESOURCE_DENSITY[] = {
    0.5, 0.3, 0.15, 0.1, 0.1, 0.08, 0.05
};

static void distribute_resources(tile_t **map, int width, int height)
{
    int total_tiles = width * height;

    for (int res = 0; res < 7; res++) {
        int count = (int)(total_tiles * RESOURCE_DENSITY[res]);
        if (count < 1)
            count = 1;

        log_info("Distributing %d %s", count,
            res == 0 ? "food" : res == 1 ? "linemate" :
            res == 2 ? "deraumere" : res == 3 ? "sibur" :
            res == 4 ? "mendiane" : res == 5 ? "phiras" : "thystame");

        for (int i = 0; i < count; i++) {
            int x = rand() % width;
            int y = rand() % height;

            if (res == 0)
                map[y][x].food++;
            else
                map[y][x].stones[res - 1]++;
        }
    }
}

tile_t **game_map_init(int width, int height, float density[])
{
    tile_t **map = malloc(sizeof(tile_t*) * height);

    if (!map)
        die("malloc map pointers");
    for (int y = 0; y < height; y++) {
        map[y] = malloc(sizeof(tile_t) * width);
        if (!map[y])
            die("malloc map row");
        memset(map[y], 0, sizeof(tile_t) * width);
    }
    distribute_resources(map, width, height);
    return map;
}

void game_map_free(tile_t **map, int height)
{
    for (int y = 0; y < height; y++)
        free(map[y]);
    free(map);
}

static void respawn_resources(server_t *srv)
{
    static int tick_count = 0;

    tick_count++;
    if (tick_count >= 20 * srv->freq) {
        tick_count = 0;
        log_info("Respawning resources");
        distribute_resources(srv->map, srv->width, srv->height);
    }
}

int check_elevation_requirements(server_t *srv, int x, int y, int level)
{
    tile_t *tile = &srv->map[y][x];
    int req_players[] = {0, 1, 2, 2, 4, 4, 6, 6};
    int req_stones[8][6] = {
        {0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0},
        {1, 1, 1, 0, 0, 0},
        {2, 0, 1, 0, 2, 0},
        {1, 1, 2, 0, 1, 0},
        {1, 2, 1, 3, 0, 0},
        {1, 2, 3, 0, 1, 0},
        {2, 2, 2, 2, 2, 1}
    };

    if (level < 1 || level > 7)
        return 0;

    int player_count = 0;
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->x == x && srv->players[i]->y == y &&
            srv->players[i]->level == level)
            player_count++;
    }

    if (player_count < req_players[level])
        return 0;

    for (int i = 0; i < 6; i++) {
        if (tile->stones[i] < req_stones[level][i])
            return 0;
    }
    return 1;
}

void perform_elevation(server_t *srv, int x, int y, int level)
{
    tile_t *tile = &srv->map[y][x];
    int req_stones[8][6] = {
        {0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0},
        {1, 1, 1, 0, 0, 0},
        {2, 0, 1, 0, 2, 0},
        {1, 1, 2, 0, 1, 0},
        {1, 2, 1, 3, 0, 0},
        {1, 2, 3, 0, 1, 0},
        {2, 2, 2, 2, 2, 1}
    };

    for (int i = 0; i < 6; i++)
        tile->stones[i] -= req_stones[level][i];

    int elevated = 0;
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->x == x && srv->players[i]->y == y &&
            srv->players[i]->level == level) {
            srv->players[i]->level++;
            srv->players[i]->is_incanting = 0;
            elevated++;
            log_info("Player #%d reached level %d",
                srv->players[i]->id, srv->players[i]->level);
        }
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "pie %d %d 1\n", x, y);
    broadcast_to_gui(srv, msg);

    int level8_count = 0;
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->level >= 8)
            level8_count++;
    }

    if (level8_count >= 6) {
        log_info("Victory! 6 players reached level 8");
        char victory_msg[256];
        snprintf(victory_msg, sizeof(victory_msg), "seg %s\n",
            srv->team_names[srv->players[0]->team_idx]);
        broadcast_to_gui(srv, victory_msg);
    }
}

static void check_incantations(server_t *srv)
{
    for (int y = 0; y < srv->height; y++) {
        for (int x = 0; x < srv->width; x++) {
            int incanting_player = -1;
            for (int i = 0; i < srv->player_count; i++) {
                if (srv->players[i]->x == x && srv->players[i]->y == y &&
                    srv->players[i]->is_incanting) {
                    incanting_player = i;
                    break;
                }
            }

            if (incanting_player >= 0) {
                player_t *p = srv->players[incanting_player];
                if (check_elevation_requirements(srv, x, y, p->level)) {
                    char msg[512];
                    snprintf(msg, sizeof(msg), "pic %d %d %d",
                        x, y, p->level);

                    for (int i = 0; i < srv->player_count; i++) {
                        if (srv->players[i]->x == x && srv->players[i]->y == y &&
                            srv->players[i]->level == p->level) {
                            char player_str[32];
                            snprintf(player_str, sizeof(player_str),
                                " #%d", srv->players[i]->id);
                            strcat(msg, player_str);
                            srv->players[i]->is_incanting = 1;
                        }
                    }
                    strcat(msg, "\n");
                    broadcast_to_gui(srv, msg);
                }
            }
        }
    }
}

void game_tick(server_t *srv)
{
    for (int i = 0; i < srv->player_count; i++)
        player_consume_life(srv->players[i], srv);

    execute_pending_actions(srv);
    check_incantations(srv);
    respawn_resources(srv);
}
