/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_add_elem_at_front
*/

#include "list.h"
#include <stdlib.h>

bool list_add_elem_at_front(list_t *front_ptr, void *elem)
{
    node_t *node = calloc(1, sizeof(node_t));
    if (node == NULL)
        return false;
    node->value = elem;
    node->next = *front_ptr;
    node->prev = NULL;
    *front_ptr = node;
    return true;
}
