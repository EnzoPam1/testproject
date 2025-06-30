/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** GUI protocol implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "server.h"
#include "gui_protocol.h"
#include "client.h"
#include "game.h"
#include "player.h"
#include "map.h"
#include "team.h"

void network_send_to_all_gui(network_t *network, const char *format, ...)
{
    char buffer[BUFFER_SIZE];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (int i = 0; i < network->client_count; i++) {
        if (network->clients[i]->type == CLIENT_GUI) {
            client_send(network->clients[i], "%s", buffer);
        }
    }
}

// GUI Command handlers
void gui_cmd_msz(server_t *server, client_t *client)
{
    client_send(client, "msz %d %d\n", 
                 server->game->map->width, 
                 server->game->map->height);
}

void gui_cmd_bct(server_t *server, client_t *client, int x, int y)
{
    if (x < 0 || x >= server->game->map->width ||
        y < 0 || y >= server->game->map->height) {
        client_send(client, "sbp\n");
        return;
    }
    
    tile_t *tile = map_get_tile(server->game->map, x, y);
    client_send(client, "bct %d %d %d %d %d %d %d %d %d\n",
                 x, y,
                 tile->resources[RES_FOOD],
                 tile->resources[RES_LINEMATE],
                 tile->resources[RES_DERAUMERE],
                 tile->resources[RES_SIBUR],
                 tile->resources[RES_MENDIANE],
                 tile->resources[RES_PHIRAS],
                 tile->resources[RES_THYSTAME]);
}

void gui_cmd_mct(server_t *server, client_t *client)
{
    for (int y = 0; y < server->game->map->height; y++) {
        for (int x = 0; x < server->game->map->width; x++) {
            gui_cmd_bct(server, client, x, y);
        }
    }
}

void gui_cmd_tna(server_t *server, client_t *client)
{
    for (int i = 0; i < server->game->team_count; i++) {
        client_send(client, "tna %s\n", server->game->teams[i]->name);
    }
}

void gui_cmd_ppo(server_t *server, client_t *client, int n)
{
    player_t *player = game_get_player_by_id(server->game, n);
    if (!player) {
        client_send(client, "sbp\n");
        return;
    }
    
    client_send(client, "ppo #%d %d %d %d\n",
                 player->id, player->x, player->y, player->orientation);
}

void gui_cmd_plv(server_t *server, client_t *client, int n)
{
    player_t *player = game_get_player_by_id(server->game, n);
    if (!player) {
        client_send(client, "sbp\n");
        return;
    }
    
    client_send(client, "plv #%d %d\n", player->id, player->level);
}

void gui_cmd_pin(server_t *server, client_t *client, int n)
{
    player_t *player = game_get_player_by_id(server->game, n);
    if (!player) {
        client_send(client, "sbp\n");
        return;
    }
    
    client_send(client, "pin #%d %d %d %d %d %d %d %d %d %d\n",
                 player->id, player->x, player->y,
                 player->inventory[RES_FOOD],
                 player->inventory[RES_LINEMATE],
                 player->inventory[RES_DERAUMERE],
                 player->inventory[RES_SIBUR],
                 player->inventory[RES_MENDIANE],
                 player->inventory[RES_PHIRAS],
                 player->inventory[RES_THYSTAME]);
}

void gui_cmd_sgt(server_t *server, client_t *client)
{
    client_send(client, "sgt %d\n", server->config->freq);
}

void gui_cmd_sst(server_t *server, client_t *client, int time)
{
    if (time < 2 || time > 10000) {
        client_send(client, "sbp\n");
        return;
    }
    
    server->config->freq = time;
    network_send_to_all_gui(server->network, "sst %d\n", time);
}

// GUI Notifications
void gui_notify_player_connect(server_t *server, player_t *player)
{
    network_send_to_all_gui(server->network, "pnw #%d %d %d %d %d %s\n",
                           player->id, player->x, player->y,
                           player->orientation, player->level,
                           server->game->teams[player->team_id]->name);
}

void gui_notify_player_position(server_t *server, player_t *player)
{
    network_send_to_all_gui(server->network, "ppo #%d %d %d %d\n",
                           player->id, player->x, player->y, player->orientation);
}

void gui_notify_player_inventory(server_t *server, player_t *player)
{
    network_send_to_all_gui(server->network, 
                           "pin #%d %d %d %d %d %d %d %d %d %d\n",
                           player->id, player->x, player->y,
                           player->inventory[RES_FOOD],
                           player->inventory[RES_LINEMATE],
                           player->inventory[RES_DERAUMERE],
                           player->inventory[RES_SIBUR],
                           player->inventory[RES_MENDIANE],
                           player->inventory[RES_PHIRAS],
                           player->inventory[RES_THYSTAME]);
}

void gui_notify_player_level(server_t *server, player_t *player)
{
    network_send_to_all_gui(server->network, "plv #%d %d\n",
                           player->id, player->level);
}

void gui_notify_player_death(server_t *server, int player_id)
{
    network_send_to_all_gui(server->network, "pdi #%d\n", player_id);
}

void gui_notify_egg_laid(server_t *server, int egg_id, int player_id, int x, int y)
{
    network_send_to_all_gui(server->network, "pfk #%d\n", player_id);
    network_send_to_all_gui(server->network, "enw #%d #%d %d %d\n",
                           egg_id, player_id, x, y);
}

void gui_notify_egg_connect(server_t *server, int egg_id)
{
    network_send_to_all_gui(server->network, "ebo #%d\n", egg_id);
}

void gui_notify_resource_collect(server_t *server, int player_id, int resource)
{
    network_send_to_all_gui(server->network, "pgt #%d %d\n",
                           player_id, resource);
}

void gui_notify_resource_drop(server_t *server, int player_id, int resource)
{
    network_send_to_all_gui(server->network, "pdr #%d %d\n",
                           player_id, resource);
}

void gui_notify_broadcast(server_t *server, int player_id, const char *message)
{
    network_send_to_all_gui(server->network, "pbc #%d %s\n",
                           player_id, message);
}

void gui_notify_incantation_start(server_t *server, int x, int y, int level,
                                 int *players, int count)
{
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer), "pic %d %d %d", x, y, level);
    
    for (int i = 0; i < count; i++) {
        len += snprintf(buffer + len, sizeof(buffer) - len, " #%d", players[i]);
    }
    
    network_send_to_all_gui(server->network, "%s\n", buffer);
}

void gui_notify_incantation_end(server_t *server, int x, int y, int result)
{
    network_send_to_all_gui(server->network, "pie %d %d %d\n", x, y, result);
}

void gui_notify_game_end(server_t *server, const char *team)
{
    network_send_to_all_gui(server->network, "seg %s\n", team);
}

void gui_notify_tile_content(server_t *server, int x, int y)
{
    tile_t *tile = map_get_tile(server->game->map, x, y);
    network_send_to_all_gui(server->network, 
                           "bct %d %d %d %d %d %d %d %d %d\n",
                           x, y,
                           tile->resources[RES_FOOD],
                           tile->resources[RES_LINEMATE],
                           tile->resources[RES_DERAUMERE],
                           tile->resources[RES_SIBUR],
                           tile->resources[RES_MENDIANE],
                           tile->resources[RES_PHIRAS],
                           tile->resources[RES_THYSTAME]);
}

void gui_notify_expulsion(server_t *server, int player_id)
{
    network_send_to_all_gui(server->network, "pex #%d\n", player_id);
}

void gui_send_initial_data(server_t *server, client_t *client)
{
    // Send map size
    gui_cmd_msz(server, client);
    
    // Send time unit
    gui_cmd_sgt(server, client);
    
    // Send all tiles
    gui_cmd_mct(server, client);
    
    // Send team names
    gui_cmd_tna(server, client);
    
    // Send all players
    for (int i = 0; i < server->game->player_count; i++) {
        player_t *player = server->game->players[i];
        
        // Player connection
        client_send(client, "pnw #%d %d %d %d %d %s\n",
                    player->id, player->x, player->y,
                    player->orientation, player->level,
                    server->game->teams[player->team_id]->name);
        
        // Player inventory
        gui_cmd_pin(server, client, player->id);
        
        // Player level
        gui_cmd_plv(server, client, player->id);
    }
    
    // Send all eggs
    for (int i = 0; i < server->game->team_count; i++) {
        team_t *team = server->game->teams[i];
        egg_t *egg = team->eggs;
        while (egg) {
            client_send(client, "enw #%d #%d %d %d\n",
                        egg->id, 0, egg->x, egg->y);
            egg = egg->next;
        }
    }
}

void process_gui_command(server_t *server, client_t *client, const char *command)
{
    char cmd[256];
    int x, y, n, time;
    
    sscanf(command, "%255s", cmd);
    
    if (strcmp(cmd, "msz") == 0) {
        gui_cmd_msz(server, client);
    } else if (strcmp(cmd, "bct") == 0) {
        if (sscanf(command, "bct %d %d", &x, &y) == 2) {
            gui_cmd_bct(server, client, x, y);
        } else {
            client_send(client, "sbp\n");
        }
    } else if (strcmp(cmd, "mct") == 0) {
        gui_cmd_mct(server, client);
    } else if (strcmp(cmd, "tna") == 0) {
        gui_cmd_tna(server, client);
    } else if (strcmp(cmd, "ppo") == 0) {
        if (sscanf(command, "ppo #%d", &n) == 1) {
            gui_cmd_ppo(server, client, n);
        } else {
            client_send(client, "sbp\n");
        }
    } else if (strcmp(cmd, "plv") == 0) {
        if (sscanf(command, "plv #%d", &n) == 1) {
            gui_cmd_plv(server, client, n);
        } else {
            client_send(client, "sbp\n");
        }
    } else if (strcmp(cmd, "pin") == 0) {
        if (sscanf(command, "pin #%d", &n) == 1) {
            gui_cmd_pin(server, client, n);
        } else {
            client_send(client, "sbp\n");
        }
    } else if (strcmp(cmd, "sgt") == 0) {
        gui_cmd_sgt(server, client);
    } else if (strcmp(cmd, "sst") == 0) {
        if (sscanf(command, "sst %d", &time) == 1) {
            gui_cmd_sst(server, client, time);
        } else {
            client_send(client, "sbp\n");
        }
    } else {
        client_send(client, "suc\n");
    }
}