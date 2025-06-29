#ifndef ZAPPY_NETWORK_H
#define ZAPPY_NETWORK_H

#include "server.h"
#include "client.h"

int  network_setup_listener(server_t *srv);
void network_handle_new_connection(server_t *srv);
void network_handle_client_io(server_t *srv, client_t *cl);

#endif // ZAPPY_NETWORK_H
