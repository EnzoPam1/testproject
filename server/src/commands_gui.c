/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** GUI commands implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include "command.h"
#include "gui_protocol.h"
#include "utils.h"

void process_gui_command(server_t *server, client_t *client, const char *command)
{
    char cmd[256];
    int x, y, n, time;
    
    sscanf(command, "%s", cmd);
    
    if (strcmp(cmd, "msz") == 0) {
        gui_cmd_msz(server, client);
    } else if (strcmp(cmd, "bct") == 0) {
        if (sscanf(command, "bct %d %d", &x, &y) == 2) {
            gui_cmd_bct(server, client, x, y);
        } else {
            network_send(client, "sbp\n");
        }
    } else if (strcmp(cmd, "mct") == 0) {
        gui_cmd_mct(server, client);
    } else if (strcmp(cmd, "tna") == 0) {
        gui_cmd_tna(server, client);
    } else if (strcmp(cmd, "ppo") == 0) {
        if (sscanf(command, "ppo #%d", &n) == 1) {
            gui_cmd_ppo(server, client, n);
        } else {
            network_send(client, "sbp\n");
        }
    } else if (strcmp(cmd, "plv") == 0) {
        if (sscanf(command, "plv #%d", &n) == 1) {
            gui_cmd_plv(server, client, n);
        } else {
            network_send(client, "sbp\n");
        }
    } else if (strcmp(cmd, "pin") == 0) {
        if (sscanf(command, "pin #%d", &n) == 1) {
            gui_cmd_pin(server, client, n);
        } else {
            network_send(client, "sbp\n");
        }
    } else if (strcmp(cmd, "sgt") == 0) {
        gui_cmd_sgt(server, client);
    } else if (strcmp(cmd, "sst") == 0) {
        if (sscanf(command, "sst %d", &time) == 1) {
            gui_cmd_sst(server, client, time);
        } else {
            network_send(client, "sbp\n");
        }
    } else {
        network_send(client, "suc\n");
    }
}