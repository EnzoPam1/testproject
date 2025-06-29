/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_get_elem_at_back
*/

#include "list.h"
#include <stdlib.h>

void *list_get_elem_at_back(list_t list)
{
    if (list == NULL)
        return NULL;
    while (list->next != NULL)
        list = list->next;
    return list->value;
}
