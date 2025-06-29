/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_get_elem_at_position
*/

#include "list.h"
#include <stdlib.h>

void *list_get_elem_at_position(list_t list, unsigned int position)
{
    for (size_t i = 0; i < position; i++) {
        if (list == NULL)
            return NULL;
        list = list->next;
    }
    if (list == NULL)
        return NULL;
    return list->value;
}
