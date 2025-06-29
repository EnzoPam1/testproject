/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Resources implementation
*/

#include <string.h>
#include "resources.h"

const float RESOURCE_DENSITY[RESOURCE_COUNT] = {
    0.5f,   // food
    0.3f,   // linemate
    0.15f,  // deraumere
    0.1f,   // sibur
    0.1f,   // mendiane
    0.08f,  // phiras
    0.05f   // thystame
};

const char *RESOURCE_NAMES[RESOURCE_COUNT] = {
    "food",
    "linemate",
    "deraumere",
    "sibur",
    "mendiane",
    "phiras",
    "thystame"
};

int resource_from_name(const char *name)
{
    for (int i = 0; i < RESOURCE_COUNT; i++) {
        if (strcmp(name, RESOURCE_NAMES[i]) == 0) {
            return i;
        }
    }
    return -1;
}

const char *resource_to_name(resource_t resource)
{
    if (resource >= 0 && resource < RESOURCE_COUNT) {
        return RESOURCE_NAMES[resource];
    }
    return NULL;
}