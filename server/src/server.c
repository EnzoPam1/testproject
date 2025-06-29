/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Server main logic
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
        "USAGE: %s -p port -x width -y height -n name1 name2 ... "
        "-c clientsNb -f freq\n", prog);
}

int server_init(server_t *srv, int argc, char **argv)
{
    int opt;
    char **team_names_tmp = NULL;
    int team_count = 0;

    srv->port = 0;
    srv->width = 0;
    srv->height = 0;
    srv->freq = 100;
    srv->teams_count = 0;
    srv->max_per_team = 0;
    srv->team_names = NULL;
    srv->slots_remaining = NULL;

    // Parser les arguments un par un pour gérer plusieurs noms d'équipes
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            srv->port = (uint16_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) {
            srv->width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-y") == 0 && i + 1 < argc) {
            srv->height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            srv->freq = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            srv->max_per_team = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-n") == 0) {
            // Collecter tous les noms d'équipes qui suivent -n
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                i++;
                team_names_tmp = realloc(team_names_tmp, sizeof(char*) * (team_count + 1));
                if (!team_names_tmp)
                    die("malloc team names");
                team_names_tmp[team_count] = strdup(argv[i]);
                team_count++;
            }
        }
    }

    if (!srv->port || !srv->width || !srv->height || !srv->freq
     || team_count == 0 || srv->max_per_team <= 0) {
        usage(argv[0]);
        return -1;
    }

    srv->teams_count = team_count;
    srv->team_names = team_names_tmp;
    srv->slots_remaining = malloc(sizeof(*srv->slots_remaining) * srv->teams_count);
    if (!srv->slots_remaining)
        die("malloc slots");

    for (int i = 0; i < srv->teams_count; i++) {
        srv->slots_remaining[i] = srv->max_per_team;
        printf("Team created: '%s' with %d slots\n", srv->team_names[i], srv->max_per_team);
    }

    float density[] = {1, 1, 1, 1, 1, 1};
    srv->map = game_map_init(srv->width, srv->height, density);

    srv->listen_fd = network_setup_listener(srv);
    if (srv->listen_fd < 0)
        die("network_setup_listener");

    srv->client_capacity = INITIAL_CLIENT_CAPACITY;
    srv->client_count = 0;
    srv->clients = malloc(sizeof(*srv->clients) * srv->client_capacity);
    if (!srv->clients)
        die("malloc clients");

    srv->player_capacity = INITIAL_CLIENT_CAPACITY;
    srv->player_count = 0;
    srv->players = malloc(sizeof(*srv->players) * srv->player_capacity);
    if (!srv->players)
        die("malloc players");

    srv->last_tick = time(NULL);
    srv->tick_count = 0;

    return 0;
}

void server_run(server_t *srv)
{
    struct timespec last, now;
    clock_gettime(CLOCK_MONOTONIC, &last);
    const double tick_duration = 1.0 / srv->freq;

    while (!stop_server) {
        int nfds = 1 + srv->client_count;
        struct pollfd fds[nfds];

        fds[0].fd = srv->listen_fd;
        fds[0].events = POLLIN;

        for (int i = 0; i < srv->client_count; i++) {
            fds[i + 1].fd = srv->clients[i]->socket_fd;
            fds[i + 1].events = POLLIN;
        }

        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - last.tv_sec) +
                        (now.tv_nsec - last.tv_nsec) / 1e9;
        int timeout = (int)((tick_duration - elapsed) * 1000);
        if (timeout < 0)
            timeout = 0;

        int ready = poll(fds, nfds, timeout);
        if (ready < 0)
            continue;

        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed = (now.tv_sec - last.tv_sec) +
                 (now.tv_nsec - last.tv_nsec) / 1e9;
        if (elapsed >= tick_duration) {
            game_tick(srv);
            last = now;
        }

        if (fds[0].revents & POLLIN)
            network_handle_new_connection(srv);

        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN)
                network_handle_client_io(srv, srv->clients[i - 1]);
        }
    }
}

void server_cleanup(server_t *srv)
{
    close(srv->listen_fd);
    game_map_free(srv->map, srv->height);

    for (int i = 0; i < srv->client_count; i++)
        client_destroy(srv->clients[i]);
    free(srv->clients);

    for (int i = 0; i < srv->player_count; i++)
        player_destroy(srv->players[i]);
    free(srv->players);

    for (int i = 0; i < srv->teams_count; i++)
        free(srv->team_names[i]);
    free(srv->team_names);
    free(srv->slots_remaining);
}
