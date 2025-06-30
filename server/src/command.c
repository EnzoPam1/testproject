/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Command implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "server.h"
#include "command.h"
#include "client.h"
#include "player.h"
#include "game.h"
#include "gui_protocol.h"
#include "utils.h"
#include "elevation.h"
#include "broadcast.h"

static void handle_client_authentication(server_t *server, client_t *client, const char *data)
{
    // Check for GUI
    if (strcmp(data, "GRAPHIC") == 0) {
        client->type = CLIENT_GUI;
        client->state = STATE_PLAYING;
        gui_send_initial_data(server, client);
        log_info("GUI client authenticated");
        return;
    }

    // Check for AI team
    team_t *team = game_get_team_by_name(server->game, data);
    if (!team) {
        client_send(client, "ko\n");
        return;
    }

    int slots = team_available_slots(team);
    if (slots <= 0) {
        client_send(client, "0\n");
        return;
    }

    // Create player
    player_t *player = game_add_player(server->game, client->fd, data);
    if (!player) {
        client_send(client, "ko\n");
        return;
    }

    // Setup client
    client->type = CLIENT_AI;
    client->state = STATE_PLAYING;
    client->player_id = player->id;
    client->team_id = team->id;
    strcpy(client->team_name, data);

    // Send connection response according to protocol
    client_send(client, "%d\n", slots - 1);
    client_send(client, "%d %d\n", server->config->width, server->config->height);

    // Notify GUI
    gui_notify_player_connect(server, player);

    log_info("Player %d joined team '%s' at (%d,%d)", 
             player->id, team->name, player->x, player->y);
}

void handle_client_command(server_t *server, client_t *client, const char *command)
{
    if (client->state == STATE_CONNECTING) {
        handle_client_authentication(server, client, command);
    } else if (client->state == STATE_PLAYING) {
        command_process(server, client, command);
    }
}

void command_process(server_t *server, client_t *client, const char *command)
{
    if (client->type == CLIENT_AI) {
        // Check if player is dead
        player_t *player = game_get_player_by_id(server->game, client->player_id);
        if (!player || player->is_dead) {
            client_send(client, "dead\n");
            return;
        }
        
        // Add to command queue if not full
        if (!client_add_command(client, command)) {
            // Queue full, ignore command
            return;
        }
        
        // Execute if no current action
        if (!client->current_action.is_active && client->cmd_queue.count == 1) {
            command_execute(server, client, player, command);
        }
    } else if (client->type == CLIENT_GUI) {
        process_gui_command(server, client, command);
    }
}

void command_execute(server_t *server, client_t *client, player_t *player, const char *command)
{
    char cmd[256] = {0};
    char arg[1024] = {0};
    
    // Parse command
    if (sscanf(command, "%255s %1023[^\n]", cmd, arg) < 1) {
        client_send(client, "ko\n");
        client_command_done(client);
        return;
    }
    
    // Execute command with duration
    if (strcmp(cmd, "Forward") == 0) {
        client_start_action(client, command, DURATION_FORWARD);
        cmd_forward(server, client, player);
    } else if (strcmp(cmd, "Right") == 0) {
        client_start_action(client, command, DURATION_TURN);
        cmd_right(server, client, player);
    } else if (strcmp(cmd, "Left") == 0) {
        client_start_action(client, command, DURATION_TURN);
        cmd_left(server, client, player);
    } else if (strcmp(cmd, "Look") == 0) {
        client_start_action(client, command, DURATION_LOOK);
        cmd_look(server, client, player);
    } else if (strcmp(cmd, "Inventory") == 0) {
        client_start_action(client, command, DURATION_INVENTORY);
        cmd_inventory(server, client, player);
    } else if (strcmp(cmd, "Broadcast") == 0) {
        client_start_action(client, command, DURATION_BROADCAST);
        cmd_broadcast(server, client, player, arg);
    } else if (strcmp(cmd, "Connect_nbr") == 0) {
        // No duration, immediate response
        cmd_connect_nbr(server, client, player);
        client_command_done(client);
        // Execute next command if any
        char *next = client_get_current_command(client);
        if (next) command_execute(server, client, player, next);
    } else if (strcmp(cmd, "Fork") == 0) {
        client_start_action(client, command, DURATION_FORK);
        cmd_fork(server, client, player);
    } else if (strcmp(cmd, "Eject") == 0) {
        client_start_action(client, command, DURATION_EJECT);
        cmd_eject(server, client, player);
    } else if (strcmp(cmd, "Take") == 0) {
        client_start_action(client, command, DURATION_TAKE);
        cmd_take(server, client, player, arg);
    } else if (strcmp(cmd, "Set") == 0) {
        client_start_action(client, command, DURATION_SET);
        cmd_set(server, client, player, arg);
    } else if (strcmp(cmd, "Incantation") == 0) {
        client_start_action(client, command, DURATION_INCANTATION);
        cmd_incantation(server, client, player);
    } else {
        client_send(client, "ko\n");
        client_command_done(client);
        // Execute next command if any
        char *next = client_get_current_command(client);
        if (next) command_execute(server, client, player, next);
    }
}

void cmd_forward(server_t *server, client_t *client, player_t *player)
{
    // Remove from current tile
    map_remove_player(server->game->map, player->x, player->y, player->id);
    
    // Move player
    player_move_forward(player, server->game->map->width, server->game->map->height);
    
    // Add to new tile
    map_add_player(server->game->map, player->x, player->y, player->id);
    
    // Response
    client_send(client, "ok\n");
    
    // Notify GUI
    gui_notify_player_position(server, player);
}

void cmd_right(server_t *server, client_t *client, player_t *player)
{
    player_turn_right(player);
    client_send(client, "ok\n");
    gui_notify_player_position(server, player);
}

void cmd_left(server_t *server, client_t *client, player_t *player)
{
    player_turn_left(player);
    client_send(client, "ok\n");
    gui_notify_player_position(server, player);
}

void cmd_look(server_t *server, client_t *client, player_t *player)
{
    char response[4096];
    strcpy(response, "[");
    int first = 1;
    
    // Calculate vision range based on level
    int range = player->level;
    
    for (int distance = 0; distance <= range; distance++) {
        int width = 2 * distance + 1;
        int start_offset = -distance;
        
        for (int offset = 0; offset < width; offset++) {
            if (!first) strcat(response, ",");
            first = 0;
            
            // Calculate relative position
            int dx = 0, dy = 0;
            int lateral = start_offset + offset;
            
            switch (player->orientation) {
                case NORTH: dx = lateral; dy = -distance; break;
                case EAST: dx = distance; dy = lateral; break;
                case SOUTH: dx = -lateral; dy = distance; break;
                case WEST: dx = -distance; dy = -lateral; break;
            }
            
            // Get absolute position with wrapping
            int x = map_wrap_x(server->game->map, player->x + dx);
            int y = map_wrap_y(server->game->map, player->y + dy);
            
            // Get tile content
            tile_t *tile = map_get_tile(server->game->map, x, y);
            int first_item = 1;
            
            // Add players
            for (int i = 0; i < tile->player_count; i++) {
                if (!first_item) strcat(response, " ");
                strcat(response, "player");
                first_item = 0;
            }
            
            // Add resources
            for (int res = 0; res < RESOURCE_COUNT; res++) {
                for (int count = 0; count < tile->resources[res]; count++) {
                    if (!first_item) strcat(response, " ");
                    strcat(response, RESOURCE_NAMES[res]);
                    first_item = 0;
                }
            }
        }
    }
    
    strcat(response, "]\n");
    client_send(client, response);
}

void cmd_inventory(server_t *server, client_t *client, player_t *player)
{
    client_send(client, "[food %d,linemate %d,deraumere %d,sibur %d,"
                        "mendiane %d,phiras %d,thystame %d]\n",
                 player->inventory[RES_FOOD],
                 player->inventory[RES_LINEMATE],
                 player->inventory[RES_DERAUMERE],
                 player->inventory[RES_SIBUR],
                 player->inventory[RES_MENDIANE],
                 player->inventory[RES_PHIRAS],
                 player->inventory[RES_THYSTAME]);
}

void cmd_broadcast(server_t *server, client_t *client, player_t *player, const char *text)
{
    broadcast_send_to_all(server->game, player, text);
    client_send(client, "ok\n");
    gui_notify_broadcast(server, player->id, text);
}

void cmd_connect_nbr(server_t *server, client_t *client, player_t *player)
{
    team_t *team = server->game->teams[player->team_id];
    client_send(client, "%d\n", team_available_slots(team));
}

void cmd_fork(server_t *server, client_t *client, player_t *player)
{
    team_t *team = server->game->teams[player->team_id];
    
    // Create egg
    egg_t *egg = team_add_egg(team, server->game->next_egg_id++, 
                             player->x, player->y);
    if (egg) {
        map_add_egg(server->game->map, player->x, player->y, egg->id);
        gui_notify_egg_laid(server, egg->id, player->id, player->x, player->y);
    }
    
    client_send(client, "ok\n");
}

void cmd_eject(server_t *server, client_t *client, player_t *player)
{
    tile_t *tile = map_get_tile(server->game->map, player->x, player->y);
    int ejected = 0;
    
    // Eject all other players
    for (int i = 0; i < tile->player_count; i++) {
        int target_id = tile->players[i];
        if (target_id == player->id) continue;
        
        player_t *target = game_get_player_by_id(server->game, target_id);
        if (!target) continue;
        
        // Calculate push direction
        int push_dir = broadcast_get_direction_from_orientation(player->orientation);
        
        // Move target
        map_remove_player(server->game->map, target->x, target->y, target->id);
        orientation_t old_orient = target->orientation;
        target->orientation = player->orientation;
        player_move_forward(target, server->game->map->width, server->game->map->height);
        target->orientation = old_orient;
        map_add_player(server->game->map, target->x, target->y, target->id);
        
        // Send eject message to target
        client_t *target_client = NULL;
        for (int j = 0; j < server->network->client_count; j++) {
            if (server->network->clients[j]->player_id == target_id) {
                target_client = server->network->clients[j];
                break;
            }
        }
        
        if (target_client) {
            client_send(target_client, "eject: %d\n", push_dir);
        }
        
        gui_notify_player_position(server, target);
        gui_notify_expulsion(server, target->id);
        ejected = 1;
    }
    
    // Eject eggs
    if (tile->egg_count > 0) {
        // Remove all eggs
        for (int i = 0; i < tile->egg_count; i++) {
            int egg_id = tile->eggs[i];
            
            // Find and remove from teams
            for (int t = 0; t < server->game->team_count; t++) {
                team_remove_egg(server->game->teams[t], egg_id);
            }
        }
        
        free(tile->eggs);
        tile->eggs = NULL;
        tile->egg_count = 0;
        ejected = 1;
    }
    
    client_send(client, ejected ? "ok\n" : "ko\n");
}

void cmd_take(server_t *server, client_t *client, player_t *player, const char *object)
{
    tile_t *tile = map_get_tile(server->game->map, player->x, player->y);
    int res = resource_from_name(object);
    
    if (res >= 0 && tile->resources[res] > 0) {
        tile->resources[res]--;
        player->inventory[res]++;
        client_send(client, "ok\n");
        gui_notify_resource_collect(server, player->id, res);
        gui_notify_player_inventory(server, player);
        gui_notify_tile_content(server, player->x, player->y);
    } else {
        client_send(client, "ko\n");
    }
}

void cmd_set(server_t *server, client_t *client, player_t *player, const char *object)
{
    tile_t *tile = map_get_tile(server->game->map, player->x, player->y);
    int res = resource_from_name(object);
    
    if (res >= 0 && player->inventory[res] > 0) {
        player->inventory[res]--;
        tile->resources[res]++;
        client_send(client, "ok\n");
        gui_notify_resource_drop(server, player->id, res);
        gui_notify_player_inventory(server, player);
        gui_notify_tile_content(server, player->x, player->y);
    } else {
        client_send(client, "ko\n");
    }
}

void cmd_incantation(server_t *server, client_t *client, player_t *player)
{
    tile_t *tile = map_get_tile(server->game->map, player->x, player->y);
    
    // Check if player is already incanting
    if (player->is_incanting) {
        client_send(client, "ko\n");
        client_command_done(client);
        return;
    }
    
    // Check requirements
    if (!elevation_check_requirements(server->game, player, tile)) {
        client_send(client, "ko\n");
        client_command_done(client);
        return;
    }
    
    // Start incantation
    player->is_incanting = true;
    client_send(client, "Elevation underway\n");
    
    // Start elevation for all eligible players
    elevation_start(server->game, player, player->x, player->y);
}