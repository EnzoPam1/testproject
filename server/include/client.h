/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Client structure and management
*/

#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "server.h"

// Client structure
struct client_s {
    int fd;
    client_type_t type;
    client_state_t state;
    
    // Network buffers
    char input_buffer[BUFFER_SIZE];
    size_t input_size;
    char output_buffer[BUFFER_SIZE];
    size_t output_size;
    
    // Command queue for AI clients
    struct {
        char *commands[MAX_COMMANDS];
        int count;
        int executing;
    } cmd_queue;
    
    // Current action timing
    struct {
        char *command;
        struct timeval start_time;
        int duration; // in time units
        bool is_active;
    } current_action;
    
    // Associated player (for AI clients)
    int player_id;
    
    // Team (for AI clients)
    int team_id;
    
    // Connection info
    char team_name[256];
};

// Client functions
client_t *client_create(int fd);
void client_destroy(client_t *client);
bool client_add_command(client_t *client, const char *command);
char *client_get_current_command(client_t *client);
void client_command_done(client_t *client);
bool client_can_send_command(client_t *client);
void client_start_action(client_t *client, const char *command, int duration);
bool client_action_done(client_t *client, int freq);
void client_send(client_t *client, const char *format, ...);

#endif /* !CLIENT_H_ */