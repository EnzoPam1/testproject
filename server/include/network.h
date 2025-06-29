/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Network management
*/

#ifndef NETWORK_H_
#define NETWORK_H_

#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_CLIENTS 1024
#define BUFFER_SIZE 4096

// Forward declarations
typedef struct client_s client_t;
typedef struct server_s server_t;

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

// Network functions
network_t *network_create(uint16_t port);
void network_destroy(network_t *network);
int network_poll(network_t *network, int timeout);
client_t *network_accept_client(network_t *network);
void network_disconnect_client(network_t *network, client_t *client);
void network_send(client_t *client, const char *format, ...);
void network_send_to_all_gui(network_t *network, const char *format, ...);
char *network_read_line(client_t *client);
void network_process_client_data(server_t *server, client_t *client);

#endif /* !NETWORK_H_ */