
#pragma once

#include "list.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

typedef struct sockaddr_in sockaddr_in_t;

typedef struct socket_s {
    int fd;
    sockaddr_in_t addr;
    char *buffer;
    size_t bufsz;
    bool handshaked;
    int fails_count;
} socket_t;

typedef struct netctl_s {
    fd_set watched_fd;
    socket_t entrypoint;
    list_t clients;
} netctl_t;

void socket_dump(const void *value);

netctl_t *netctl_new(int port);
int netctl_accept(netctl_t *netctl);
void netctl_disconnect(netctl_t *netctl, int fd);
void netctl_destroy(netctl_t *netctl);
