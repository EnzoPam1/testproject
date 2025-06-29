
#include "list.h"
#include <stdlib.h>

void list_sort(list_t *begin, cmp_function_t cmp)
{
    if (begin == NULL || *begin == NULL)
        return;
    list_t sorted = NULL;
    list_t head = *begin;
    while (head != NULL)
    {
        list_t next = head->next;
        list_t *sorted_ptr = &sorted;
        while (*sorted_ptr != NULL && cmp(head->value, (*sorted_ptr)->value) > 0)
            sorted_ptr = &(*sorted_ptr)->next;
        head->next = *sorted_ptr;
        *sorted_ptr = head;
        head = next;
    }
    *begin = sorted;
}
