/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Resources definition
*/

#ifndef RESOURCES_H_
#define RESOURCES_H_

// Resource types
typedef enum {
    RES_FOOD = 0,
    RES_LINEMATE,
    RES_DERAUMERE,
    RES_SIBUR,
    RES_MENDIANE,
    RES_PHIRAS,
    RES_THYSTAME,
    RESOURCE_COUNT
} resource_t;

// Resource densities for spawning
extern const float RESOURCE_DENSITY[RESOURCE_COUNT];

// Resource names
extern const char *RESOURCE_NAMES[RESOURCE_COUNT];

// Resource functions
int resource_from_name(const char *name);
const char *resource_to_name(resource_t resource);

#endif /* !RESOURCES_H_ */