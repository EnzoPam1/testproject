/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_get_elem_at_front
*/

#include "list.h"
#include <stdlib.h>

void *list_get_elem_at_front(list_t list)
{
    if (list == NULL)
        return NULL;
    return list->value;
}
