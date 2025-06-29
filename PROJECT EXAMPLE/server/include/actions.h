/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Player actions header
*/

#ifndef ACTIONS_H_
    #define ACTIONS_H_

    #include "server.h"
    #include "client.h"
    #include "player.h"

void execute_pending_actions(server_t *srv);
void broadcast_to_gui(server_t *srv, const char *msg);

#endif /* !ACTIONS_H_ */
