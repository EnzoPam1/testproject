#include "typedef.h"
#include "server.h"
#include "actions.h"
#include "list.h"
#include "game.h"

timeval_t *player_get_next_food_timeout(list_t players, int freq)
{
    if (players == NULL)
        return NULL;
    timeval_t *timeout = calloc(1, sizeof(timeval_t));
    int food_timer_units = ((player_t *)players->value)->entity->food_timer_units;
    for (list_t head = players->next; head != NULL; head = head->next)
    {
        player_t *player = (player_t *)head->value;
        if (strncmp(player->team_name, "GRAPHIC", 7) == 0)
            continue;
        food_timer_units = player->entity->food_timer_units < food_timer_units
                               ? player->entity->food_timer_units
                               : food_timer_units;
    }
    timeout->tv_usec = ((double)food_timer_units / freq) * 1000000;
    if (timeout->tv_usec == 0)
    {
        free(timeout);
        return NULL;
    }
    return timeout;
}

timeval_t *actions_get_next_timeout(list_t *action_list, int frequency)
{
    if (*action_list == NULL)
        return NULL;
    timeval_t *timeout = calloc(1, sizeof(timeval_t));

    action_t *next_action = (action_t *)(*action_list)->value;
    list_t head = *action_list;
    while (head != NULL) {
        action_t *action = (action_t *)head->value;
        if (action->cooldown < next_action->cooldown)
            next_action = action;
        head = head->next;
    }
    timeout->tv_usec = ((double)next_action->cooldown / frequency) * 1000000;
    return timeout;
}

timeval_t *ressources_get_next_timeout(game_t *game, int frequency)
{
    timeval_t *timeout = calloc(1, sizeof(timeval_t));
    timeout->tv_usec = ((double)game->ressources_time_unit / frequency) * 1000000;
    return timeout;
}

timeval_t *server_get_next_timeout(server_t *server)
{
    timeval_t *action_timeout = actions_get_next_timeout(&server->actions, server->game->freq);
    timeval_t *pfood_timeout = player_get_next_food_timeout(server->game->players, server->game->freq);
    timeval_t *ressources_timeout = ressources_get_next_timeout(server->game, server->game->freq);

    timeval_t *next = ressources_timeout;
    if (action_timeout != NULL && (action_timeout->tv_usec < next->tv_usec)) {
        free(next);
        next = action_timeout;
    }
    else
        free(action_timeout);

    if (pfood_timeout != NULL && (pfood_timeout->tv_usec < next->tv_usec)) {
        free(next);
        next = pfood_timeout;
    }
    else
        free(pfood_timeout);

    return next;
}
