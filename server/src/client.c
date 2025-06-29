/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Client management
*/

#include "client.h"
#include "buffer.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

client_t *client_create(int socket_fd)
{
    client_t *cl = malloc(sizeof(*cl));

    if (!cl)
        die("malloc client");
    memset(cl, 0, sizeof(*cl));
    cl->socket_fd = socket_fd;
    cl->state = STATE_INIT;
    cl->is_gui = 0;
    cl->team_idx = -1;
    cl->id = 0;
    buffer_init(&cl->buf);
    return cl;
}

void client_destroy(client_t *cl)
{
    if (!cl)
        return;
    if (cl->socket_fd >= 0)
        close(cl->socket_fd);
    free(cl);
}
