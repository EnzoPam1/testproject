/*
** EPITECH PROJECT, 2023
** B-PDG-300-MAR-3-1-PDGD04pm-erwan.gonzales
** File description:
** list_dump
*/

#include "list.h"
#include <stdlib.h>
#include <stdio.h>

void list_dump(list_t list, value_displayer_t val_disp)
{
    while (list != NULL) {
        val_disp(list->value);
        list = list->next;
    }
}
