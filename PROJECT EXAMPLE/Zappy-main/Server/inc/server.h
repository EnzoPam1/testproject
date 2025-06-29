/*
** EPITECH PROJECT, 2023
** B-YEP-400-MAR-4-1-zappy-erwan.gonzales
** File description:
** server
*/

#pragma once

#include "typedef.h"
#include "list.h"
#include "netctl.h"
#include "options.h"
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

typedef struct server_s
{
    option_t *options;
    netctl_t *netctl;
game_t *game;
    list_t actions;
} server_t;

server_t *server_new(int argc, char *argv[]);
void server_select(server_t *server);
void server_destroy(server_t *server);
bool server_handshake(server_t *server, socket_t *s);
bool server_process_buffer(server_t *server, socket_t *s);
void server_process_activity(server_t *server, fd_set *readfds, int act);
int server_run(server_t *server);
timeval_t *server_get_next_timeout(server_t *server);
