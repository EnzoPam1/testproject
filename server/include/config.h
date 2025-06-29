/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Configuration constants and macros
*/

#ifndef CONFIG_H_
    #define CONFIG_H_

// Network configuration
#define MAX_CLIENTS 100
#define MAX_PLAYERS 1000
#define BUFFER_SIZE 4096
#define MAX_COMMAND_LENGTH 1024
#define MAX_TEAMS 10
#define MAX_TEAM_NAME_LENGTH 50

// Game configuration
#define MIN_MAP_SIZE 10
#define MAX_MAP_SIZE 100
#define MIN_FREQUENCY 1
#define MAX_FREQUENCY 10000
#define DEFAULT_FREQUENCY 100

// Player configuration
#define INITIAL_FOOD 10
#define LIFE_UNIT_DURATION 126
#define MAX_INVENTORY_SLOTS 7
#define MAX_LEVEL 8

// Action timeouts (in time units)
#define TIMEOUT_FORWARD 7
#define TIMEOUT_TURN 7
#define TIMEOUT_LOOK 7
#define TIMEOUT_INVENTORY 1
#define TIMEOUT_BROADCAST 7
#define TIMEOUT_TAKE 7
#define TIMEOUT_SET 7
#define TIMEOUT_FORK 42
#define TIMEOUT_EJECT 7
#define TIMEOUT_INCANTATION 300

// Resource configuration
#define RESOURCE_COUNT 7
#define RESOURCE_RESPAWN_INTERVAL 20

// Vision configuration
#define BASE_VISION_SIZE 1

// Debug and logging
#define DEBUG_MODE 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DEBUG 2
#define LOG_LEVEL_ERROR 3

// Protocol constants
#define WELCOME_MESSAGE "WELCOME\n"
#define GRAPHIC_TEAM_NAME "GRAPHIC"
#define RESPONSE_OK "ok\n"
#define RESPONSE_KO "ko\n"
#define RESPONSE_DEAD "dead\n"

// Error codes
#define ERROR_SUCCESS 0
#define ERROR_INVALID_ARGS 84
#define ERROR_NETWORK 1
#define ERROR_MEMORY 2
#define ERROR_GAME_LOGIC 3

#endif /* !CONFIG_H_ */