/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Network management
*/

#ifndef NETWORK_H_
#define NETWORK_H_

#include "server.h"
#include "client.h"

// Network functions
void network_process_client_data(server_t *server, client_t *client);
void network_send_to_all_gui(network_t *network, const char *format, ...);
char *client_read_line(client_t *client);

#endif /* !NETWORK_H_ */