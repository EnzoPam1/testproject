
#pragma once

#include "typedef.h"
#include "list.h"
#include "game.h"

typedef struct action_s {
    enum actions_e {
        ACTION_FORWARD,
        ACTION_LEFT,
        ACTION_RIGHT,
        ACTION_LOOK,
        ACTION_INVENTORY,
        ACTION_BROADCAST,
        ACTION_CONNECT_NBR,
        ACTION_FORK,
        ACTION_EJECT,
        ACTION_TAKE,
        ACTION_SET,
        ACTION_INCANTATION,
    } type;
    int cooldown;
    int issuer;
    char name[64];
    char arg[1024];
    const char *(*callback)(game_t *game, player_t *player, const char *arg);
} action_t;

/**
 * Action functions
*/
action_t *action_new(int issuer, const char *cmd);
int action_cmp_cooldown(const void *a, const void *b);
void action_dump(const void *value);

/**
 * Actions list functions
*/
bool actions_accept(list_t *action_list, action_t *action);
timeval_t *actions_get_next_timeout(list_t *action_list, int frequency);
void actions_apply_elapsed_time(list_t action_list, size_t elapsed_time);
void actions_remove_from_issuer(list_t *action_list, int issuer);
