/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Team implementation
*/

#include <stdlib.h>
#include <string.h>
#include "team.h"

team_t *team_create(int id, const char *name, int max_clients)
{
    team_t *team = calloc(1, sizeof(team_t));
    if (!team) return NULL;

    team->id = id;
    team->name = strdup(name);
    team->max_clients = max_clients;
    team->egg_count = 0;
    team->connected_clients = 0;
    team->eggs = NULL;

    return team;
}

void team_destroy(team_t *team)
{
    if (!team) return;

    // Free eggs
    egg_t *egg = team->eggs;
    while (egg) {
        egg_t *next = egg->next;
        free(egg);
        egg = next;
    }

    free(team->name);
    free(team);
}

int team_available_slots(team_t *team)
{
    return team->egg_count;
}

egg_t *team_add_egg(team_t *team, int egg_id, int x, int y)
{
    egg_t *egg = calloc(1, sizeof(egg_t));
    if (!egg) return NULL;

    egg->id = egg_id;
    egg->team_id = team->id;
    egg->x = x;
    egg->y = y;
    egg->next = team->eggs;
    team->eggs = egg;
    team->egg_count++;

    return egg;
}

void team_remove_egg(team_t *team, int egg_id)
{
    egg_t **current = &team->eggs;
    
    while (*current) {
        if ((*current)->id == egg_id) {
            egg_t *to_remove = *current;
            *current = (*current)->next;
            free(to_remove);
            team->egg_count--;
            return;
        }
        current = &(*current)->next;
    }
}

egg_t *team_get_random_egg(team_t *team)
{
    if (!team->eggs || team->egg_count == 0) return NULL;

    int index = rand() % team->egg_count;
    egg_t *egg = team->eggs;
    
    for (int i = 0; i < index && egg; i++) {
        egg = egg->next;
    }
    
    return egg;
}