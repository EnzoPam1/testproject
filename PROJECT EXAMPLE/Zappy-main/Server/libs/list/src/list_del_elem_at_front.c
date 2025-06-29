/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_del_elem_at_front
*/

#include "list.h"
#include <stdlib.h>
#include <stdio.h>
bool list_del_elem_at_front(list_t *front_ptr)
{
    if (*front_ptr == NULL)
        return false;
    list_t head = *front_ptr;
    *front_ptr = head->next;
    if (head->next != NULL)
        head->next->prev = head->prev;
    if (head->prev != NULL)
        head->prev->next = head->next;
    free(head);
    return true;
}
