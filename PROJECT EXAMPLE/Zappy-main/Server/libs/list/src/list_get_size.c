/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_get_size
*/

#include "list.h"
#include <stdlib.h>

unsigned int list_get_size(list_t list)
{
    unsigned int size = 0;
    while (list != NULL) {
        list = list->next;
        size++;
    }
    return size;
}
