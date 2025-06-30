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
#include <sys/time.h>
#include <poll.h>

#define MAX_CLIENTS 1024
#define BUFFER_SIZE 4096
#define MAX_COMMANDS 10

// Forward declarations
typedef struct server_s server_t;
typedef struct game_s game_t;
typedef struct network_s network_t;
typedef struct client_s client_t;
typedef struct player_s player_t;

// Action durations in time units
#define DURATION_FORWARD 7
#define DURATION_TURN 7
#define DURATION_LOOK 7
#define DURATION_INVENTORY 1
#define DURATION_BROADCAST 7
#define DURATION_TAKE 7
#define DURATION_SET 7
#define DURATION_EJECT 7
#define DURATION_FORK 42
#define DURATION_INCANTATION 300

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

// Client types
typedef enum {
    CLIENT_UNKNOWN = 0,
    CLIENT_AI,
    CLIENT_GUI
} client_type_t;

// Client states
typedef enum {
    STATE_CONNECTING = 0,
    STATE_CONNECTED,
    STATE_PLAYING
} client_state_t;

// Network structure
typedef struct network_s {
    int listen_fd;
    struct pollfd *poll_fds;
    int poll_count;
    int poll_capacity;
    client_t **clients;
    int client_count;
    int client_capacity;
} network_t;

// Main server structure
struct server_s {
    config_t *config;
    game_t *game;
    network_t *network;
    bool running;
    struct timeval start_time;
    struct timeval last_tick;
    double tick_accumulator;
};

// Server functions
server_t *server_create(int argc, char **argv);
void server_destroy(server_t *server);
int server_run(server_t *server);
void server_stop(server_t *server);
void handle_client_command(server_t *server, client_t *client, const char *command);

// Global server instance for signal handling
extern server_t *g_server;

#endif /* !SERVER_H_ */