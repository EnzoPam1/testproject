/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Client implementation
*/

#include <stdlib.h>
#include <string.h>
#include "client.h"

client_t *client_create(int fd)
{
    client_t *client = calloc(1, sizeof(client_t));
    if (!client) return NULL;

    client->fd = fd;
    client->type = CLIENT_UNKNOWN;
    client->state = STATE_CONNECTING;
    client->player_id = -1;
    client->team_id = -1;

    return client;
}

void client_destroy(client_t *client)
{
    if (!client) return;

    // Free command queue
    for (int i = 0; i < client->cmd_queue.count; i++) {
        free(client->cmd_queue.commands[i]);
    }

    free(client);
}

bool client_add_command(client_t *client, const char *command)
{
    if (client->cmd_queue.count >= MAX_COMMANDS) {
        return false;  // Queue full
    }

    client->cmd_queue.commands[client->cmd_queue.count] = strdup(command);
    client->cmd_queue.count++;
    
    return true;
}

char *client_get_current_command(client_t *client)
{
    if (client->cmd_queue.executing >= client->cmd_queue.count) {
        return NULL;
    }
    
    return client->cmd_queue.commands[client->cmd_queue.executing];
}

void client_command_done(client_t *client)
{
    if (client->cmd_queue.executing >= client->cmd_queue.count) {
        return;
    }

    // Free executed command
    free(client->cmd_queue.commands[client->cmd_queue.executing]);

    // Shift commands
    for (int i = client->cmd_queue.executing; i < client->cmd_queue.count - 1; i++) {
        client->cmd_queue.commands[i] = client->cmd_queue.commands[i + 1];
    }

    client->cmd_queue.count--;
    
    // Keep executing index at 0 to process next command
    client->cmd_queue.executing = 0;
}

bool client_can_send_command(client_t *client)
{
    return client->cmd_queue.count < MAX_COMMANDS;
}