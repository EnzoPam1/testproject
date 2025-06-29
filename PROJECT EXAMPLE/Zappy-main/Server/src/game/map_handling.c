
#include "game.h"

int to_supp(float nbr)
{
    if ((nbr - (int) nbr) < 0.0001)
        return nbr;
    return (int) ceilf(nbr);
}

int get_map_ressources(game_t *game,int index)
{
    int res = 0;
    for (int i = 0; i < game->map->size.y ; i++) {
        for (int a = 0; a < game->map->size.x ; a++) {
            res += get_ressource(game, a, i, index);
        }
    }
    return res;
}

int get_ressource(game_t *game, int x, int y, int index)
{
    return game->map->tiles[y][x].ressources[index];
}

int get_tile_ressource(game_t *game, int x, int y)
{
    int res = 0;
    for (int i = 0; i < 7; i++) {
        res += game->map->tiles[y][x].ressources[i];
    }
    return res;
}

void print_ressources(game_t *game)
{
    for (int y = 0 ; y < game->map->size.y ; y++) {
        for (int x = 0 ; x < game->map->size.x ; x++) {
            for (int i = 0 ; i < 7 ; i++) {
                printf("%d, ", game->map->tiles[y][x].ressources[i]);
            }
            printf("\n");
        }
    }
}

char *get_ressource_name(game_t *game, char *ressource_in, int x, int y)
{
    int before = 0;
    static const char *mineral_tab[] = {"food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"};
    for (int i = 0, ressource = 0 ; i < 7 ; i++) {
        ressource = 0;
        while (ressource < game->map->tiles[y][x].ressources[i]) {
            ressource_in = realloc(ressource_in, sizeof(char) * (strlen(ressource_in) +
            (strlen(mineral_tab[i]) + 2)));
            if (before != 0)
                strcat(ressource_in, " ");
            strcat(ressource_in, mineral_tab[i]);
            ressource++;
            before = 1;
        }
    }
    return ressource_in;
}

int normalize(int x, int x_max)
{
    x = x % x_max;
    if (x < 0)
        x = x_max + x;
    return x;
}

game_t *add_in_map(game_t *game, int index, int ressource_to_add)
{
    int randx = 0, randy = 0;
    for (int tolerance = 0 ; ressource_to_add > 0 ; tolerance++) {
        for (int i = 0 ; i < (game->map->size.y * game->map->size.x) ; i++) {
            randx = rand() % game->map->size.x;
            randy = rand() % game->map->size.y;
            if (get_tile_ressource(game, randx, randy) <= tolerance) {
                game->map->tiles[randy][randx].ressources[index] += 1;
                ressource_to_add--;
            }
            if (ressource_to_add == 0) {
                return game;
            }
        }
    }
    return game;
}

game_t *spawn_ressources(game_t *game)
{
    float density[7] = {0.5, 0.3, 0.15, 0.1, 0.1,0.08, 0.05};
    float ressources_available = 0;
    int ressources_to_add = 0;

    for (int i = 0; i < 7 ; i++) {
        ressources_available = get_map_ressources(game, i);
        ressources_to_add = to_supp(game->map->size.x * game->map->size.y * density[i] - ressources_available);
        add_in_map(game, i, ressources_to_add);
    }
    return game;
}
