/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Server main header
*/

#ifndef SERVER_H_
#define SERVER_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// Forward declarations
typedef struct server_s server_t;
typedef struct game_s game_t;
typedef struct network_s network_t;
typedef struct timer_s timer_t;

// Server configuration
typedef struct config_s {
    uint16_t port;
    int width;
    int height;
    int clients_nb;
    int freq;
    char **team_names;
    int team_count;
} config_t;

// Main server structure
struct server_s {
    config_t *config;
    game_t *game;
    network_t *network;
    timer_t *timer;
    bool running;
    time_t start_time;
};

// Server functions
server_t *server_create(int argc, char **argv);
void server_destroy(server_t *server);
int server_run(server_t *server);
void server_stop(server_t *server);

// Global server instance for signal handling
extern server_t *g_server;

#endif /* !SERVER_H_ */