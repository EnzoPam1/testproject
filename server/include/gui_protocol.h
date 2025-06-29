/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** GUI protocol
*/

#ifndef GUI_PROTOCOL_H_
#define GUI_PROTOCOL_H_

// Forward declarations
typedef struct server_s server_t;
typedef struct client_s client_t;
typedef struct player_s player_t;
typedef struct tile_s tile_t;

// GUI command handlers
void gui_cmd_msz(server_t *server, client_t *client);
void gui_cmd_bct(server_t *server, client_t *client, int x, int y);
void gui_cmd_mct(server_t *server, client_t *client);
void gui_cmd_tna(server_t *server, client_t *client);
void gui_cmd_ppo(server_t *server, client_t *client, int n);
void gui_cmd_plv(server_t *server, client_t *client, int n);
void gui_cmd_pin(server_t *server, client_t *client, int n);
void gui_cmd_sgt(server_t *server, client_t *client);
void gui_cmd_sst(server_t *server, client_t *client, int time);

// GUI notifications
void gui_notify_player_connect(server_t *server, player_t *player);
void gui_notify_player_position(server_t *server, player_t *player);
void gui_notify_player_inventory(server_t *server, player_t *player);
void gui_notify_player_level(server_t *server, player_t *player);
void gui_notify_player_death(server_t *server, int player_id);
void gui_notify_egg_laid(server_t *server, int egg_id, int player_id, int x, int y);
void gui_notify_egg_connect(server_t *server, int egg_id);
void gui_notify_resource_collect(server_t *server, int player_id, int resource);
void gui_notify_resource_drop(server_t *server, int player_id, int resource);
void gui_notify_broadcast(server_t *server, int player_id, const char *message);
void gui_notify_incantation_start(server_t *server, int x, int y, int level, int *players, int count);
void gui_notify_incantation_end(server_t *server, int x, int y, int result);
void gui_notify_game_end(server_t *server, const char *team);
void gui_notify_tile_content(server_t *server, int x, int y);
void gui_notify_expulsion(server_t *server, int player_id);

// GUI utilities
void gui_send_initial_data(server_t *server, client_t *client);

#endif /* !GUI_PROTOCOL_H_ */