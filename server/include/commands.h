#ifndef ZAPPY_COMMANDS_H
#define ZAPPY_COMMANDS_H

#include "server.h"
#include "client.h"

/**
 * @brief Interprète et traite une ligne de commande reçue d'un client.
 *
 * @param srv  Le serveur
 * @param cl   Le client émetteur
 * @param line La ligne reçue (avec '\n')
 */
void dispatch_command(server_t *srv, client_t *cl, const char *line);

#endif // ZAPPY_COMMANDS_H
