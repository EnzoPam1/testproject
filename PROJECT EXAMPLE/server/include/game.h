/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Game logic header
*/

#ifndef GAME_H_
    #define GAME_H_

typedef struct s_server server_t;

typedef struct s_tile {
    int food;
    int stones[6];
} tile_t;

tile_t **game_map_init(int width, int height, float density[]);
void game_map_free(tile_t **map, int height);
void game_tick(server_t *srv);
int check_elevation_requirements(server_t *srv, int x, int y, int level);
void perform_elevation(server_t *srv, int x, int y, int level);

#endif /* !GAME_H_ */
