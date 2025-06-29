#include "netctl.h"
#include <stdio.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void netctl_disconnect(netctl_t *netctl, int fd)
{
    for (list_t *head = &netctl->clients; *head != NULL; head = &(*head)->next)
    {
        socket_t *socket = (socket_t *)(*head)->value;
        if (socket->fd == fd)
        {
            close(socket->fd);
            FD_CLR(socket->fd, &netctl->watched_fd);
            list_del_elem_at_front(head);
            free(socket->buffer);
            free(socket);
            return;
        }
    }
}

int netctl_accept(netctl_t *netctl)
{
    socket_t *socket = calloc(1, sizeof(socket_t));
    socklen_t clilen = sizeof(socket->addr);

    socket->fd = accept(netctl->entrypoint.fd, (struct sockaddr *)&socket->addr, &clilen);
    if (socket->fd < 0)
    {
        perror("ERROR on accept");
        exit(EXIT_FAILURE);
    }

    FD_SET(socket->fd, &netctl->watched_fd);
    list_add_elem_at_back(&netctl->clients, socket);

    return socket->fd;
}

void socket_dump(const void *value)
{
    socket_t *socket = (socket_t *)value;
    printf("Socket: fd=%d\n", socket->fd);
}

netctl_t *netctl_new(int port)
{
    netctl_t *netctl = calloc(1, sizeof(netctl_t));

    netctl->entrypoint.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (netctl->entrypoint.fd < 0)
    {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }
    netctl->entrypoint.addr.sin_family = AF_INET;
    netctl->entrypoint.addr.sin_addr.s_addr = INADDR_ANY;
    netctl->entrypoint.addr.sin_port = htons(port);

    int optval = 1;
    if (setsockopt(netctl->entrypoint.fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    if (bind(netctl->entrypoint.fd, (struct sockaddr *)&netctl->entrypoint.addr, sizeof(netctl->entrypoint.addr)) < 0)
    {
        perror("ERROR on binding");
        exit(EXIT_FAILURE);
    }

    listen(netctl->entrypoint.fd, 25);

    FD_ZERO(&netctl->watched_fd);
    FD_SET(netctl->entrypoint.fd, &netctl->watched_fd);

    return netctl;
}

void netctl_destroy(netctl_t *netctl)
{
    for (list_t head = netctl->clients; head != NULL; head = head->next)
    {
        close(((socket_t *)head->value)->fd);
        free(head->value);
    }
    list_clear(&netctl->clients);
    close(netctl->entrypoint.fd);
    free(netctl);
}
