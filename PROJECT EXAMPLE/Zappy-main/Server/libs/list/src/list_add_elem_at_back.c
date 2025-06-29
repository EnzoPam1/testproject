/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_add_elem_at_back
*/

#include "list.h"
#include <stdlib.h>

bool list_add_elem_at_back(list_t *front_ptr, void *elem)
{
    if (*front_ptr == NULL)
        return list_add_elem_at_front(front_ptr, elem);
    node_t *node = calloc(1, sizeof(node_t));
    if (node == NULL)
        return false;
    list_t head = *front_ptr;
    while (head->next != NULL)
        head = head->next;
    node->value = elem;
    node->next = NULL;
    node->prev = head;
    head->next = node;
    return true;
}
