/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Server structure and functions
*/

#pragma once

#include <signal.h>
#include <stdint.h>
#include <time.h>
#include "game.h"
#include "client.h"
#include "player.h"

typedef struct s_server {
    uint16_t   port;
    int        width, height;
    int        freq;
    int        listen_fd;

    char     **team_names;
    int        teams_count;
    int        max_per_team;
    int       *slots_remaining;

    client_t **clients;
    int        client_count;
    int        client_capacity;

    player_t **players;
    int        player_count;
    int        player_capacity;

    tile_t   **map;

    time_t     last_tick;
    int        tick_count;
} server_t;

extern volatile sig_atomic_t stop_server;

int  server_init(server_t *srv, int argc, char **argv);
void server_run(server_t *srv);
void server_cleanup(server_t *srv);
void handle_signal(int sig);
