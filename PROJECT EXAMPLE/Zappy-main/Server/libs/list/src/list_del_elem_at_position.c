/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_del_elem_at_position
*/

#include "list.h"
#include <stdlib.h>

bool list_del_elem_at_position(list_t *front_ptr, unsigned int position)
{
    if (*front_ptr == NULL)
        return false;
    if (position == 0)
        return list_del_elem_at_front(front_ptr);
    list_t head = *front_ptr;
    for (size_t i = 0; i < position - 1; i++) {
        if (head->next == NULL)
            return false;
        head = head->next;
    }
    if (head->next == NULL)
        return false;
    list_t buff = head->next->next;
    free(head->next);
    head->next = buff;
    if (buff != NULL)
        head->next->prev = head;
    return true;
}
