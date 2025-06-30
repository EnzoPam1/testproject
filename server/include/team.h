/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Team management
*/

#ifndef TEAM_H_
#define TEAM_H_

// Forward declarations
typedef struct egg_s egg_t;
typedef struct team_s team_t;

// Egg structure
typedef struct egg_s {
    int id;
    int team_id;
    int x;
    int y;
    struct egg_s *next;
} egg_t;

// Team structure
typedef struct team_s {
    int id;
    char *name;
    int max_clients;
    egg_t *eggs;
    int egg_count;
    int connected_clients;
} team_t;

// Team functions
team_t *team_create(int id, const char *name, int max_clients);
void team_destroy(team_t *team);
int team_available_slots(team_t *team);
egg_t *team_add_egg(team_t *team, int egg_id, int x, int y);
void team_remove_egg(team_t *team, int egg_id);
egg_t *team_get_random_egg(team_t *team);

#endif /* !TEAM_H_ */