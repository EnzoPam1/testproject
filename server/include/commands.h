/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Command processing
*/

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "server.h"
#include "network.h"

// Command processing
void process_command(server_t *server, client_t *client, const char *command);
void process_ai_command(server_t *server, client_t *client, player_t *player, const char *command);
void process_gui_command(server_t *server, client_t *client, const char *command);

// AI Commands
void cmd_forward(server_t *server, client_t *client, player_t *player);
void cmd_right(server_t *server, client_t *client, player_t *player);
void cmd_left(server_t *server, client_t *client, player_t *player);
void cmd_look(server_t *server, client_t *client, player_t *player);
void cmd_inventory(server_t *server, client_t *client, player_t *player);
void cmd_broadcast(server_t *server, client_t *client, player_t *player, const char *message);
void cmd_connect_nbr(server_t *server, client_t *client, player_t *player);
void cmd_fork(server_t *server, client_t *client, player_t *player);
void cmd_eject(server_t *server, client_t *client, player_t *player);
void cmd_take(server_t *server, client_t *client, player_t *player, const char *object);
void cmd_set(server_t *server, client_t *client, player_t *player, const char *object);
void cmd_incantation(server_t *server, client_t *client, player_t *player);

#endif /* !COMMANDS_H_ */