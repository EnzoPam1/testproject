/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Client structure
*/

#ifndef CLIENT_H_
    #define CLIENT_H_

    #include <stdint.h>
    #include "buffer.h"

typedef enum {
    STATE_INIT,
    STATE_AUTH,
    STATE_ACTIVE
} client_state_t;

typedef struct s_client {
    int             socket_fd;
    client_state_t  state;
    int             is_gui;
    int             team_idx;
    int             id;
    buffer_t        buf;
} client_t;

client_t *client_create(int socket_fd);
void      client_destroy(client_t *cl);

#endif /* !CLIENT_H_ */
