/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Broadcast system
*/

#ifndef BROADCAST_H_
#define BROADCAST_H_

#include "player.h"
#include "game.h"

// Broadcast functions
int broadcast_get_direction(player_t *sender, player_t *receiver, int map_width, int map_height);
int broadcast_get_direction_from_orientation(orientation_t orientation);
void broadcast_send_to_all(game_t *game, player_t *sender, const char *message);

#endif /* !BROADCAST_H_ */