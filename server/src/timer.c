/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Timer implementation
*/

#include <stdlib.h>
#include <time.h>
#include "timer.h"

timer_t *timer_create(int freq)
{
    timer_t *timer = calloc(1, sizeof(timer_t));
    if (!timer) return NULL;
    
    timer->freq = freq;
    clock_gettime(CLOCK_MONOTONIC, &timer->last_tick);
    timer->accumulated = 0.0;
    
    return timer;
}

void timer_destroy(timer_t *timer)
{
    free(timer);
}

bool timer_should_tick(timer_t *timer)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    double elapsed = (now.tv_sec - timer->last_tick.tv_sec) +
                    (now.tv_nsec - timer->last_tick.tv_nsec) / 1e9;
    
    double tick_duration = 1.0 / timer->freq;
    
    if (elapsed >= tick_duration) {
        timer->last_tick = now;
        return true;
    }
    
    return false;
}

int timer_get_timeout(timer_t *timer)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    double elapsed = (now.tv_sec - timer->last_tick.tv_sec) +
                    (now.tv_nsec - timer->last_tick.tv_nsec) / 1e9;
    
    double tick_duration = 1.0 / timer->freq;
    double remaining = tick_duration - elapsed;
    
    if (remaining <= 0) return 0;
    
    return (int)(remaining * 1000);  // Convert to milliseconds
}

void timer_update(timer_t *timer)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    double elapsed = (now.tv_sec - timer->last_tick.tv_sec) +
                    (now.tv_nsec - timer->last_tick.tv_nsec) / 1e9;
    
    timer->accumulated += elapsed * timer->freq;
}