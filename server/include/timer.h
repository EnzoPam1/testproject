/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Timer management
*/

#ifndef TIMER_H_
#define TIMER_H_

#include <time.h>
#include <stdbool.h>

// Timer structure
typedef struct timer_s {
    int freq;
    struct timespec last_tick;
    double accumulated;
} timer_t;

// Timer functions
timer_t *timer_create(int freq);
void timer_destroy(timer_t *timer);
bool timer_should_tick(timer_t *timer);
int timer_get_timeout(timer_t *timer);
void timer_update(timer_t *timer);

#endif /* !TIMER_H_ */