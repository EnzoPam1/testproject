/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_clear
*/

#include "list.h"
#include "stdlib.h"

void list_clear(list_t *front_ptr)
{
    list_t head = *front_ptr;
    while (head != NULL) {
        list_t buff = head->next;
        free(head);
        head = buff;
    }
    *front_ptr = NULL;
}
