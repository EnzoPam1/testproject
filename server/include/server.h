/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Command processing
*/

#ifndef COMMAND_H_
#define COMMAND_H_

// Forward declarations
typedef struct server_s server_t;
typedef struct client_s client_t;
typedef struct player_s player_t;

// Command durations (in time units)
#define DURATION_FORWARD 7
#define DURATION_TURN 7
#define DURATION_LOOK 7
#define DURATION_INVENTORY 1
#define DURATION_BROADCAST 7
#define DURATION_FORK 42
#define DURATION_EJECT 7
#define DURATION_TAKE 7
#define DURATION_SET 7
#define DURATION_INCANTATION 300

// Command processing
void command_process(server_t *server, client_t *client, const char *command);
void command_execute(server_t *server, client_t *client, player_t *player, const char *command);

// AI commands
void cmd_forward(server_t *server, client_t *client, player_t *player);
void cmd_right(server_t *server, client_t *client, player_t *player);
void cmd_left(server_t *server, client_t *client, player_t *player);
void cmd_look(server_t *server, client_t *client, player_t *player);
void cmd_inventory(server_t *server, client_t *client, player_t *player);
void cmd_broadcast(server_t *server, client_t *client, player_t *player, const char *text);
void cmd_connect_nbr(server_t *server, client_t *client, player_t *player);
void cmd_fork(server_t *server, client_t *client, player_t *player);
void cmd_eject(server_t *server, client_t *client, player_t *player);
void cmd_take(server_t *server, client_t *client, player_t *player, const char *object);
void cmd_set(server_t *server, client_t *client, player_t *player, const char *object);
void cmd_incantation(server_t *server, client_t *client, player_t *player);

#endif /* !COMMAND_H_ */