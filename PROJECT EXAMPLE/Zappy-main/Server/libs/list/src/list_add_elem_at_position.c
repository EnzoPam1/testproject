/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_add_elem_at_position
*/

#include "list.h"
#include <stdlib.h>

bool list_add_elem_at_position(
    list_t *front_ptr, void *elem, unsigned int position)
{
    if (position == 0)
        return list_add_elem_at_front(front_ptr, elem);
    list_t head = *front_ptr;
    for (size_t i = 0; i < position - 1; i++) {
        if (head == NULL)
            return false;
        head = head->next;
    }
    if (head == NULL)
        return false;
    node_t *node = calloc(1, sizeof(node_t));
    if (node == NULL)
        return false;
    node->value = elem;
    node->next = head->next;
    node->prev = head;
    head->next = node;
    return true;
}
