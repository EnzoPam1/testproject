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

#define MAX_COMMANDS 10

// Client types
typedef enum {
    CLIENT_UNKNOWN = 0,
    CLIENT_AI,
    CLIENT_GUI
} client_type_t;

// Client states
typedef enum {
    STATE_CONNECTING = 0,
    STATE_CONNECTED,
    STATE_PLAYING
} client_state_t;

// Client structure
typedef struct client_s {
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
        int executing;  // Index of currently executing command
    } cmd_queue;
    
    // Associated player (for AI clients)
    int player_id;
    
    // Team (for AI clients)
    int team_id;
    
} client_t;

// Client functions
client_t *client_create(int fd);
void client_destroy(client_t *client);
bool client_add_command(client_t *client, const char *command);
char *client_get_current_command(client_t *client);
void client_command_done(client_t *client);
bool client_can_send_command(client_t *client);

#endif /* !CLIENT_H_ */