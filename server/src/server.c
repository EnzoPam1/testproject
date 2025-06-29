/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Server main logic - Fixed for AI compatibility
*/

#include "server.h"
#include "network.h"
#include "game.h"
#include "utils.h"
#include "client.h"
#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <poll.h>
#include <time.h>

#define INITIAL_CLIENT_CAPACITY 10

static void usage(const char *prog)
{
    fprintf(stderr,
        "USAGE: %s -p port -x width -y height -n name1,name2,... "
        "-c clientsNb -f freq\n", prog);
    fprintf(stderr, "  -p port      Port number\n");
    fprintf(stderr, "  -x width     Width of the world\n");
    fprintf(stderr, "  -y height    Height of the world\n");
    fprintf(stderr, "  -n teams     Team names separated by commas\n");
    fprintf(stderr, "  -c clientsNb Number of authorized clients per team\n");
    fprintf(stderr, "  -f freq      Reciprocal of time unit (default: 100)\n");
}

static int parse_teams(server_t *srv, const char *teams_str)
{
    if (!teams_str) return -1;
    
    char *teams_copy = strdup(teams_str);
    if (!teams_copy) return -1;
    
    // Count teams
    srv->teams_count = 1;
    for (char *p = teams_copy; *p; p++) {
        if (*p == ',') srv->teams_count++;
    }
    
    // Allocate arrays
    srv->team_names = malloc(sizeof(char*) * srv->teams_count);
    srv->slots_remaining = malloc(sizeof(int) * srv->teams_count);
    
    if (!srv->team_names || !srv->slots_remaining) {
        free(teams_copy);
        return -1;
    }
    
    // Parse team names
    int idx = 0;
    char *token = strtok(teams_copy, ",");
    while (token && idx < srv->teams_count) {
        srv->team_names[idx] = strdup(token);
        srv->slots_remaining[idx] = srv->max_per_team;
        idx++;
        token = strtok(NULL, ",");
    }
    
    free(teams_copy);
    return 0;
}

int server_init(server_t *srv, int argc, char **argv)
{
    int opt;
    char *teams_str = NULL;

    // Initialize defaults
    memset(srv, 0, sizeof(server_t));
    srv->port = 0;
    srv->width = 0;
    srv->height = 0;
    srv->freq = 100;
    srv->teams_count = 0;
    srv->max_per_team = 0;
    srv->team_names = NULL;
    srv->slots_remaining = NULL;
    srv->listen_fd = -1;

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "p:x:y:f:n:c:h")) != -1) {
        switch (opt) {
        case 'p': 
            srv->port = (uint16_t)atoi(optarg); 
            break;
        case 'x': 
            srv->width = atoi(optarg); 
            break;
        case 'y': 
            srv->height = atoi(optarg); 
            break;
        case 'f': 
            srv->freq = atoi(optarg); 
            break;
        case 'n': 
            teams_str = optarg; 
            break;
        case 'c': 
            srv->max_per_team = atoi(optarg); 
            break;
        case 'h':
            usage(argv[0]);
            return 1;
        default:
            usage(argv[0]);
            return -1;
        }
    }

    // Validate required parameters
    if (!srv->port || !srv->width || !srv->height || !srv->freq
     || !teams_str || srv->max_per_team <= 0) {
        fprintf(stderr, "Error: Missing required parameters\n");
        usage(argv[0]);
        return -1;
    }
    
    // Validate parameter ranges
    if (srv->width < 10 || srv->width > 100) {
        fprintf(stderr, "Error: Width must be between 10 and 100\n");
        return -1;
    }
    if (srv->height < 10 || srv->height > 100) {
        fprintf(stderr, "Error: Height must be between 10 and 100\n");
        return -1;
    }
    if (srv->freq < 1 || srv->freq > 10000) {
        fprintf(stderr, "Error: Frequency must be between 1 and 10000\n");
        return -1;
    }

    // Parse teams
    if (parse_teams(srv, teams_str) < 0) {
        fprintf(stderr, "Error: Failed to parse team names\n");
        return -1;
    }

    // Initialize game map
    float density[] = {0.5, 0.3, 0.15, 0.1, 0.1, 0.08, 0.05};
    srv->map = game_map_init(srv->width, srv->height, density);
    if (!srv->map) {
        fprintf(stderr, "Error: Failed to initialize game map\n");
        return -1;
    }

    // Setup network
    srv->listen_fd = network_setup_listener(srv);
    if (srv->listen_fd < 0) {
        fprintf(stderr, "Error: Failed to setup network listener\n");
        return -1;
    }

    // Initialize client management
    srv->client_capacity = INITIAL_CLIENT_CAPACITY;
    srv->client_count = 0;
    srv->clients = malloc(sizeof(client_t*) * srv->client_capacity);
    if (!srv->clients) {
        fprintf(stderr, "Error: Failed to allocate client array\n");
        return -1;
    }

    // Initialize player management
    srv->player_capacity = INITIAL_CLIENT_CAPACITY;
    srv->player_count = 0;
    srv->players = malloc(sizeof(player_t*) * srv->player_capacity);
    if (!srv->players) {
        fprintf(stderr, "Error: Failed to allocate player array\n");
        return -1;
    }

    // Initialize timing
    srv->last_tick = time(NULL);
    srv->tick_count = 0;

    log_info("Server initialized successfully:");
    log_info("  Port: %d", srv->port);
    log_info("  Map size: %dx%d", srv->width, srv->height);
    log_info("  Frequency: %d", srv->freq);
    log_info("  Teams: %d", srv->teams_count);
    for (int i = 0; i < srv->teams_count; i++) {
        log_info("    %s (%d slots)", srv->team_names[i], srv->max_per_team);
    }

    return 0;
}

void server_run(server_t *srv)
{
    struct timespec last, now;
    clock_gettime(CLOCK_MONOTONIC, &last);
    const double tick_duration = 1.0 / srv->freq;

    log_info("Server started, entering main loop");

    while (!stop_server) {
        // Prepare poll array
        int nfds = 1 + srv->client_count;
        struct pollfd fds[nfds];

        fds[0].fd = srv->listen_fd;
        fds[0].events = POLLIN;

        for (int i = 0; i < srv->client_count; i++) {
            fds[i + 1].fd = srv->clients[i]->socket_fd;
            fds[i + 1].events = POLLIN;
        }

        // Calculate timeout for next tick
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - last.tv_sec) +
                        (now.tv_nsec - last.tv_nsec) / 1e9;
        int timeout = (int)((tick_duration - elapsed) * 1000);
        if (timeout < 0) timeout = 0;

        // Poll for events
        int ready = poll(fds, nfds, timeout);
        if (ready < 0 && !stop_server) {
            log_info("Poll error, continuing...");
            continue;
        }

        // Check if it's time for a game tick
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed = (now.tv_sec - last.tv_sec) +
                 (now.tv_nsec - last.tv_nsec) / 1e9;
        if (elapsed >= tick_duration) {
            game_tick(srv);
            last = now;
        }

        // Handle new connections
        if (ready > 0 && (fds[0].revents & POLLIN)) {
            network_handle_new_connection(srv);
        }

        // Handle client I/O
        if (ready > 0) {
            for (int i = 1; i < nfds && i <= srv->client_count; i++) {
                if (fds[i].revents & POLLIN) {
                    // Find the client by file descriptor
                    for (int j = 0; j < srv->client_count; j++) {
                        if (srv->clients[j]->socket_fd == fds[i].fd) {
                            network_handle_client_io(srv, srv->clients[j]);
                            break;
                        }
                    }
                }
            }
        }

        // Clean up dead players periodically
        static int cleanup_counter = 0;
        if (++cleanup_counter >= 1000) { // Every ~1000 iterations
            network_cleanup_dead_players(srv);
            cleanup_counter = 0;
        }
    }

    log_info("Server shutting down...");
}

void server_cleanup(server_t *srv)
{
    if (!srv) return;

    log_info("Cleaning up server resources...");

    // Close network
    if (srv->listen_fd >= 0) {
        close(srv->listen_fd);
    }
    
    network_cleanup(srv);

    // Free game map
    if (srv->map) {
        game_map_free(srv->map, srv->height);
    }

    // Free clients
    if (srv->clients) {
        for (int i = 0; i < srv->client_count; i++) {
            if (srv->clients[i]) {
                client_destroy(srv->clients[i]);
            }
        }
        free(srv->clients);
    }

    // Free players
    if (srv->players) {
        for (int i = 0; i < srv->player_count; i++) {
            if (srv->players[i]) {
                player_destroy(srv->players[i]);
            }
        }
        free(srv->players);
    }

    // Free team data
    if (srv->team_names) {
        for (int i = 0; i < srv->teams_count; i++) {
            if (srv->team_names[i]) {
                free(srv->team_names[i]);
            }
        }
        free(srv->team_names);
    }
    
    if (srv->slots_remaining) {
        free(srv->slots_remaining);
    }

    log_info("Server cleanup completed");
}