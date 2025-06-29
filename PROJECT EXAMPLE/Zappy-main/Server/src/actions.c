#include "actions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void accumulate_cooldown(list_t action_list, action_t *action);

static const char *actions[] = {
    "Forward",
    "Right",
    "Left",
    "Look",
    "Inventory",
    "Broadcast ",
    "Connect_nbr",
    "Fork",
    "Eject",
    "Take ",
    "Set ",
    "Incantation",
    NULL};

static const int cooldowns[] = {
    7,  // Forward
    7,  // Right
    7,  // Left
    7,  // Look
    1,  // Inventory
    7,  // Broadcast
    0,  // Connect_nbr
    42, // Fork
    7,  // Eject
    7,  // Take
    7,  // Set
    300 // Incantation
};

static const char *(*callbacks[])(game_t *, player_t *, const char *) = {
    &move_forward,
    &turn_right,
    &turn_left,
    &look,
    &get_inventory,
    &broadcast,
    &team_unused_slots,
    &fork_player,
    &eject_player,
    &take_object,
    &drop_object,
    &resolve_incantation};

/**
 * TODO: certaines actions doivent etre traités quand on récupére l'action du joueur, au tout débu du cd; ex: incantation fork;
 */
action_t *action_new(int issuer, const char *cmd)
{
    action_t *action = calloc(1, sizeof(action_t));
    action->issuer = issuer;
    for (int i = 0; actions[i] != NULL; i++)
    {
        if (strncmp(cmd, actions[i], strlen(actions[i])) == 0)
        {
            strcpy(action->name, actions[i]);
            action->type = i;
            action->cooldown = cooldowns[action->type];
            action->callback = callbacks[action->type];
            if (action->type == ACTION_BROADCAST || action->type == ACTION_TAKE || action->type == ACTION_SET)
                strncpy(action->arg, cmd + strlen(actions[i]), strlen(cmd) - strlen(actions[i]));
            return action;
        }
    }
    free(action);
    return NULL;
}

int action_cmp_cooldown(const void *a, const void *b)
{
    action_t *action_a = (action_t *)a;
    action_t *action_b = (action_t *)b;
    if (action_a->cooldown < action_b->cooldown)
        return -1;
    if (action_a->cooldown > action_b->cooldown)
        return 1;
    return 0;
}

bool actions_accept(list_t *action_list, action_t *action)
{
    if (action == NULL)
        return false;
    int occ = 0;
    for (list_t head = *action_list; head != NULL; head = head->next)
    {
        action_t *a = (action_t *)head->value;
        if (a->issuer == action->issuer)
            occ++;
    }
    if (occ < 10)
    {
        accumulate_cooldown(*action_list, action);
        list_add_elem_at_back(action_list, action);
        return true;
    }
    free(action);
    return false;
}

void actions_apply_elapsed_time(list_t action_list, size_t elapsed_time)
{
    for (list_t head = action_list; head != NULL; head = head->next)
    {
        action_t *action = (action_t *)head->value;
        action->cooldown -= elapsed_time;
    }
}

void actions_remove_from_issuer(list_t *action_list, int issuer)
{
    for (list_t *head = action_list; *head != NULL;)
    {
        action_t *action = (action_t *)(*head)->value;
        if (action->issuer == issuer)
        {
            list_del_elem_at_front(head);
            free(action);
            continue;
        }
        head = &(*head)->next;
    }
}

static void accumulate_cooldown(list_t action_list, action_t *action)
{
    int max_cooldown = 0;
    for (list_t head = action_list; head != NULL; head = head->next)
    {
        action_t *a = (action_t *)head->value;
        if (a->issuer == action->issuer)
            max_cooldown = a->cooldown > max_cooldown ? a->cooldown : max_cooldown;
    }
    action->cooldown += max_cooldown;
}

void action_dump(const void *value)
{
    if (value == NULL) {
        printf("Action: NULL\n");
    }
    else
    {
        action_t *action = (action_t *)value;
        printf("Action: %s <%s>, cd=%d\n", action->name, action->arg, action->cooldown);
    }
}