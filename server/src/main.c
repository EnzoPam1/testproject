/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Main entry point
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "server.h"
#include "utils.h"

server_t *g_server = NULL;

static void signal_handler(int sig)
{
    (void)sig;
    if (g_server) {
        g_server->running = false;
    }
}

static void print_usage(const char *prog)
{
    printf("USAGE: %s -p port -x width -y height -n name1 name2 ... "
           "-c clientsNb -f freq\n", prog);
    printf("\tport\t\tis the port number\n");
    printf("\twidth\t\tis the width of the world\n");
    printf("\theight\t\tis the height of the world\n");
    printf("\tnameX\t\tis the name of the team X\n");
    printf("\tclientsNb\tis the number of authorized clients per team\n");
    printf("\tfreq\t\tis the reciprocal of time unit for execution of actions\n");
}

int main(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "-help") == 0) {
        print_usage(argv[0]);
        return (argc < 2) ? 84 : 0;
    }

    // Create server
    g_server = server_create(argc, argv);
    if (!g_server) {
        return 84;
    }

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Run server
    int ret = server_run(g_server);

    // Cleanup
    server_destroy(g_server);
    return ret;
}