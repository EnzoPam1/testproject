/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Network handling
*/

#include "network.h"
#include "buffer.h"
#include "commands.h"
#include "utils.h"
#include "client.h"
#include "server.h"
#include "player.h"
#include "actions.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

int network_setup_listener(server_t *srv)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(srv->port),
        .sin_addr = {.s_addr = INADDR_ANY}
    };

    if (fd < 0)
        return -1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        return -1;
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, 10) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void sendf(int fd, const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    send(fd, buf, strlen(buf), 0);
}

static void remove_client(server_t *srv, int idx)
{
    client_t *cl = srv->clients[idx];

    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->client_idx == cl->id) {
            char msg[256];
            snprintf(msg, sizeof(msg), "pdi #%d\n", srv->players[i]->id);
            broadcast_to_gui(srv, msg);

            player_destroy(srv->players[i]);
            for (int j = i + 1; j < srv->player_count; j++)
                srv->players[j - 1] = srv->players[j];
            srv->player_count--;
            break;
        }
    }

    close(cl->socket_fd);
    client_destroy(cl);
    for (int j = idx + 1; j < srv->client_count; j++)
        srv->clients[j - 1] = srv->clients[j];
    srv->client_count--;
}

void network_handle_new_connection(server_t *srv)
{
    struct sockaddr_in peer;
    socklen_t len = sizeof(peer);
    int fd = accept(srv->listen_fd, (struct sockaddr*)&peer, &len);
    static int next_client_id = 1;

    if (fd < 0)
        return;

    log_info("New client connected (fd=%d)", fd);
    client_t *cl = client_create(fd);
    buffer_init(&cl->buf);
    cl->state = STATE_AUTH;
    cl->id = next_client_id++;

    if (srv->client_count == srv->client_capacity) {
        int newcap = srv->client_capacity * 2;
        client_t **tmp = realloc(srv->clients, sizeof(*tmp) * newcap);
        if (!tmp)
            die("realloc clients");
        srv->clients = tmp;
        srv->client_capacity = newcap;
    }
    srv->clients[srv->client_count++] = cl;

    sendf(fd, "WELCOME\n");
}

void network_handle_client_io(server_t *srv, client_t *cl)
{
    char tmp[512], line[BUF_SIZE];
    ssize_t r = recv(cl->socket_fd, tmp, sizeof(tmp), 0);

    if (r <= 0) {
        for (int i = 0; i < srv->client_count; i++) {
            if (srv->clients[i] == cl) {
                log_info("Client disconnected (fd=%d)", cl->socket_fd);
                remove_client(srv, i);
                break;
            }
        }
        return;
    }

    buffer_write(&cl->buf, tmp, (size_t)r);

    while (buffer_readline(&cl->buf, line, sizeof(line))) {
        line[strcspn(line, "\r\n")] = '\0';

        if (cl->state == STATE_AUTH) {
            int handled = 0;
            for (int i = 0; i < srv->teams_count; i++) {
                if (strcmp(line, srv->team_names[i]) == 0) {
                    if (srv->slots_remaining[i] > 0) {
                        cl->is_gui = 0;
                        cl->team_idx = i;
                        cl->state = STATE_ACTIVE;
                        srv->slots_remaining[i]--;

                        sendf(cl->socket_fd, "%d %d %d\n", srv->slots_remaining[i], srv->width, srv->height);
                        int spawn_x = rand() % srv->width;
                        int spawn_y = rand() % srv->height;
                        player_t *player = player_create(cl->id, i,
                            spawn_x, spawn_y);

                        if (srv->player_count == srv->player_capacity) {
                            int newcap = srv->player_capacity * 2;
                            player_t **tmp = realloc(srv->players,
                                sizeof(*tmp) * newcap);
                            if (!tmp)
                                die("realloc players");
                            srv->players = tmp;
                            srv->player_capacity = newcap;
                        }
                        srv->players[srv->player_count++] = player;

                        char msg[256];
                        snprintf(msg, sizeof(msg),
                            "pnw #%d %d %d %d %d %s\n",
                            player->id, player->x, player->y,
                            player->orientation, player->level,
                            srv->team_names[i]);
                        broadcast_to_gui(srv, msg);
                    } else {
                        sendf(cl->socket_fd, "0\n");
                        for (int j = 0; j < srv->client_count; j++) {
                            if (srv->clients[j] == cl) {
                                remove_client(srv, j);
                                break;
                            }
                        }
                        return;
                    }
                    handled = 1;
                    break;
                }
            }
            if (!handled && strcmp(line, "GRAPHIC") == 0) {
                cl->is_gui = 1;
                cl->state = STATE_ACTIVE;
                handled = 1;

                sendf(cl->socket_fd, "msz %d %d\n", srv->width, srv->height);
                for (int y = 0; y < srv->height; y++) {
                    for (int x = 0; x < srv->width; x++) {
                        tile_t *tile = &srv->map[y][x];
                        sendf(cl->socket_fd, "bct %d %d %d %d %d %d %d %d %d\n",
                            x, y, tile->food, tile->stones[0], tile->stones[1],
                            tile->stones[2], tile->stones[3], tile->stones[4],
                            tile->stones[5]);
                    }
                }
                for (int i = 0; i < srv->teams_count; i++)
                    sendf(cl->socket_fd, "tna %s\n", srv->team_names[i]);

                for (int i = 0; i < srv->player_count; i++) {
                    player_t *p = srv->players[i];
                    sendf(cl->socket_fd, "pnw #%d %d %d %d %d %s\n",
                        p->id, p->x, p->y, p->orientation, p->level,
                        srv->team_names[p->team_idx]);
                }

                sendf(cl->socket_fd, "sgt %d\n", srv->freq);
            }
            if (!handled) {
                sendf(cl->socket_fd, "0\n");
                for (int j = 0; j < srv->client_count; j++) {
                    if (srv->clients[j] == cl) {
                        remove_client(srv, j);
                        break;
                    }
                }
            }
        } else {
            dispatch_command(srv, cl, line);
        }
    }
}
