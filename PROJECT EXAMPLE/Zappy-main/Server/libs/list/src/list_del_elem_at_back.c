/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_del_elem_at_back
*/

#include "list.h"
#include <stdlib.h>

bool list_del_elem_at_back(list_t *front_ptr)
{
    if (*front_ptr == NULL)
        return false;
    list_t head = *front_ptr;
    if (head->next == NULL) {
        free(head);
        *front_ptr = NULL;
        return true;
    }
    while (head->next->next != NULL)
        head = head->next;
    free(head->next);
    head->next = NULL;
    return true;
}
