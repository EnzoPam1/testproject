#include "game.h"

static const int for_level[7][8] = {
    // nb player, level_needed, linemate, deraumere, sibur, mendiane, phiras, thystame
    {1, 1, 1, 0, 0, 0, 0, 0},
    {2, 2, 1, 1, 1, 0, 0, 0},
    {2, 3, 2, 0, 1, 0, 2, 0},
    {4, 4, 1, 1, 2, 0, 1, 0},
    {4, 5, 1, 2, 1, 3, 0, 0},
    {6, 6, 1, 2, 3, 0, 1, 0},
    {6, 7, 2, 2, 2, 2, 2, 1}};

static const char *mineral_tab[] = {
    "linemate",
    "deraumere",
    "sibur",
    "mendiane",
    "phiras",
    "thystame"};

const char *move_forward(game_t *game, player_t *player, const char *arg)
{
    if (player == NULL) {
        return "ko\n";
    }
    if (player->entity->orientation.x == 1)
        player->entity->pos.x = normalize(player->entity->pos.x + 1, game->map->size.x);
    else if (player->entity->orientation.x == -1)
        player->entity->pos.x = normalize(player->entity->pos.x - 1, game->map->size.x);
    else if (player->entity->orientation.y == 1)
        player->entity->pos.y = normalize(player->entity->pos.y + 1, game->map->size.y);
    else if (player->entity->orientation.y == -1)
        player->entity->pos.y = normalize(player->entity->pos.y - 1, game->map->size.y);
    gui_player_position(game, player);
    gui_send_all(game, game->send_message);
    return ("ok\n");
}

const char *turn_right(game_t *game, player_t *player, const char *arg)
{
    if (player == NULL)
        return "ko\n";
    if (player->entity->orientation.x == 0 && player->entity->orientation.y == 1) {
        player->entity->orientation.x = -1;
        player->entity->orientation.y = 0;
    }
    else if (player->entity->orientation.x == -1 && player->entity->orientation.y == 0)
    {
        player->entity->orientation.x = 0;
        player->entity->orientation.y = -1;
    }
    else if (player->entity->orientation.x == 0 && player->entity->orientation.y == -1)
    {
        player->entity->orientation.x = 1;
        player->entity->orientation.y = 0;
    }
    else if (player->entity->orientation.x == 1 && player->entity->orientation.y == 0)
    {
        player->entity->orientation.x = 0;
        player->entity->orientation.y = 1;
    }
    gui_player_position(game, player);
    gui_send_all(game, game->send_message);
    return "ok\n";
}

const char *turn_left(game_t *game ,player_t *player, const char *arg)
{
    if (player == NULL)
        return "ko\n";
    if (player->entity->orientation.x == 0 && player->entity->orientation.y == 1) {
        player->entity->orientation.x = 1;
        player->entity->orientation.y = 0;
    }
    else if (player->entity->orientation.x == 1 && player->entity->orientation.y == 0)
    {
        player->entity->orientation.x = 0;
        player->entity->orientation.y = -1;
    }
    else if (player->entity->orientation.x == 0 && player->entity->orientation.y == -1)
    {
        player->entity->orientation.x = -1;
        player->entity->orientation.y = 0;
    }
    else if (player->entity->orientation.x == -1 && player->entity->orientation.y == 0)
    {
        player->entity->orientation.x = 0;
        player->entity->orientation.y = 1;
    }
    gui_player_position(game, player);
    gui_send_all(game, game->send_message);
    return "ok\n";
}

const char *team_unused_slots(game_t *game, player_t *player, const char *arg)
{
    team_t *team = get_team_by_name(game, player->team_name);
    if (team == NULL)
        return "ko\n";
    memset(game->buffer, 0, BUFSIZ);
    sprintf(game->buffer, "%d\n", team->max_players - team->nb_players);
    return game->buffer;
}

void decrease_food_left(game_t *game)
{
    for (list_t ptr = game->players; ptr != NULL; ptr = ptr->next)
    {
        player_t *player = ptr->value;
        if (strcmp(player->team_name, "GRAPHIC") == 0)
            continue;
        player->entity->food_left -= 1;
    }
}

const char *get_inventory(game_t *game, player_t *player, const char *arg)
{
    char *inventory = calloc(1, sizeof(char) * 20);
    memset(inventory, 0, 20);
    char num_buffer[10];
    memset(num_buffer, 0, 10);
    sprintf(inventory, "[food %d,", player->entity->food_left);
    for (int i = 0; i < 6; i++)
    {
        sprintf(num_buffer, "%d", player->entity->minerals[i]);
        inventory = realloc(inventory, sizeof(char) * (strlen(inventory) + strlen(mineral_tab[i]) + strlen(num_buffer) + 5));
        strcat(inventory, mineral_tab[i]);
        strcat(inventory, " ");
        strcat(inventory, num_buffer);
        memset(num_buffer, 0, 10);
        if (i != 5)
            strcat(inventory, ",");
    }
    strcat(inventory, "]\n");
    return inventory;
}

int get_mineral_index(const char *arg)
{
    for (int i = 0; i < 6; i++) {
        if (strcmp(mineral_tab[i], arg) == 0)
            return i;
    }
    return -100;
}

const char *take_object(game_t *game, player_t *player, const char *arg)
{
    int x = player->entity->pos.x;
    int y = player->entity->pos.y;
    if (strncmp(arg, "food", 4) == 0 && game->map->tiles[y][x].ressources[0] > 0) {
        game->map->tiles[y][x].ressources[0] -= 1;
        player->entity->food_left += 1;
        gui_pgt(game, player, -1);
        gui_send_all(game, game->send_message);
        gui_player_inventory(game, player);
        gui_send_all(game, game->send_message);
        return "ok\n";
    }
    int index = get_mineral_index(arg);
    if (index == -100)
        return "ko\n";
    if (game->map->tiles[player->entity->pos.y][player->entity->pos.x].ressources[index + 1] > 0) {
        game->map->tiles[player->entity->pos.y][player->entity->pos.x].ressources[index + 1]--;
        player->entity->minerals[index]++;
        //gui communication
        gui_pgt(game, player, index);
        gui_send_all(game, game->send_message);
        gui_player_inventory(game, player);
        gui_send_all(game, game->send_message);
        return "ok\n";
    }
    return "ko\n";
}

const char *drop_object(game_t *game, player_t *player, const char *arg)
{
    int x = player->entity->pos.x;
    int y = player->entity->pos.y;
    if (strncmp(arg, "food", 4) == 0 && player->entity->food_left > 0) {
        game->map->tiles[y][x].ressources[0] += 1;
        player->entity->food_left -= 1;
        gui_pgt(game, player, -1);
        gui_send_all(game, game->send_message);
        gui_player_inventory(game, player);
        gui_send_all(game, game->send_message);
        return "ok\n";
    }
    int index = get_mineral_index(arg);
    if (index == -100)
        return "ko\n";
    if (player->entity->minerals[index] > 0) {
        game->map->tiles[player->entity->pos.y][player->entity->pos.x].ressources[index + 1]++;
        player->entity->minerals[index]--;
        //gui communication
        gui_pdr(game, player, index);
        gui_send_all(game, game->send_message);
        gui_player_inventory(game, player);
        gui_send_all(game, game->send_message);
        return "ok\n";
    }
    return "ko\n";
}

int check_incantation_players(game_t *game, player_t *player, int players_needed, int level_required)
{
    int nb_players = 1;
    incantation_t *incantation = get_incantation_by_player(game, player);
    for (list_t player = incantation->casters; player != NULL; player = player->next)
    {
        player_t *player2 = player->value;
        if (player2->entity->level == level_required)
        {
            nb_players++;
        }
    }
    if (nb_players >= players_needed)
    {
        return 0;
    }
    return 1;
}

int check_players(game_t *game, player_t *player, int players_needed, int level_required)
{
    int nb_players = 0;
    for (list_t ptr = game->players; ptr != NULL; ptr = ptr->next)
    {
        player_t *player2 = ptr->value;
        if (strcmp(player2->team_name, "GRAPHIC") == 0)
            continue;
        if (player2->entity->pos.x == player->entity->pos.x && player2->entity->pos.y == player->entity->pos.y)
        {
            if (player2->entity->level == level_required)
            {
                nb_players++;
            }
        }
    }
    if (nb_players >= players_needed)
    {
        return 0;
    }
    return 1;
}

const char *resolve_incantation(game_t *game, player_t *player, const char *arg)
{
    if (strncmp(verif_incantation(game, player, arg, 1), "ko", strlen("ko")) == 0) {
        return "ko\n";
    }
    int lvl = player->entity->level - 1;
    for (int i = 0; i < 6; i++)
    {
        game->map->tiles[player->entity->pos.y][player->entity->pos.x].ressources[i + 1] -= for_level[lvl][i + 2];
    }
    gui_tile_content(game, player->entity->pos.x, player->entity->pos.y);
    gui_send_all(game, game->send_message);
    player->entity->level++;
    gui_pie(game, player, player->entity->level);
    gui_send_all(game, game->send_message);
    incantation_t *incantation = get_incantation_by_player(game, player);
    if (incantation == NULL)
        return "ko\n";
    for (list_t ptr = incantation->casters; ptr != NULL; ptr = ptr->next)
    {
        player_t *player2 = ptr->value;
        if (player2->entity->pos.x == player->entity->pos.x && player2->entity->pos.y == player->entity->pos.y)
        {
            player2->entity->is_incantating = 0;
            player2->entity->level++;
            gui_pie(game, player, player2->entity->level);
            gui_send_all(game, game->send_message);
            if (player2->fd != player->fd) {
                dprintf(player2->fd, "Current level: %d\n", player2->entity->level);
            }
        }
    }
    //gui communication
    memset(game->buffer, 0, BUFSIZ);
    sprintf(game->buffer, "Current level: %d\n", player->entity->level);
    remove_incantation_by_player(game, player);
    return game->buffer;
}

const char *verif_incantation(game_t *game, player_t *player, const char *arg, int state)
{
    int lvl = player->entity->level - 1;
    int status = 0;
    if (state == 0)
        status = check_players(game, player, for_level[lvl][0], for_level[lvl][1]);
    else
        status = check_incantation_players(game, player, for_level[lvl][0], for_level[lvl][1]);
    if (status == 1)
        return "ko\n";
    for (int i = 0; i < 6; i++) {
        if (game->map->tiles[player->entity->pos.y][player->entity->pos.x].ressources[i + 1] < for_level[lvl][i + 2])
            return "ko\n";
    }
    return "Elevation underway\n";
}

char *get_player_in_tile(game_t *game, char *ressources, int x, int y)
{
    for (list_t ptr = game->players; ptr != NULL; ptr = ptr->next) {
        player_t *player = ptr->value;
        if (strcmp(player->team_name, "GRAPHIC") == 0)
            continue;
        if (player->entity->pos.x == x && player->entity->pos.y == y) {
            ressources = realloc(ressources, sizeof(char) * (strlen(ressources) + 8));
            strcat(ressources, "player ");
        }
    }
    return ressources;
}

char *get_egg_in_tile(game_t *game, char *ressources, int x, int y)
{
    for (list_t ptr = game->eggs; ptr != NULL; ptr = ptr->next) {
        egg_t *egg = ptr->value;
        if (egg->pos.x == x && egg->pos.y == y) {
            ressources = realloc(ressources, sizeof(char) * (strlen(ressources) + 4));
            strcat(ressources, "egg ");
        }
    }
    return ressources;
}

const char *look(game_t *game, player_t *player, const char *arg)
{
    char *ressources = calloc(1, sizeof(char) * 2);
    memset(ressources, 0, 2);
    memcpy(ressources, "[", 1);
    ressources = get_player_in_tile(game, ressources, player->entity->pos.x, player->entity->pos.y);
    ressources = get_egg_in_tile(game, ressources, player->entity->pos.x, player->entity->pos.y);
    ressources = get_ressource_name(game, ressources, player->entity->pos.x, player->entity->pos.y);
    int x_mult = (player->entity->orientation.x != 0) ? ((player->entity->orientation.x > 0) ? 1 : -1) : 0;
    int y_mult = (player->entity->orientation.y != 0) ? ((player->entity->orientation.y > 0) ? 1 : -1) : 0;
    int current_view_size = 0, start_x = 0, start_y = 0;
    for (int i = 0 ; i < player->entity->level ; i++) {
        current_view_size = 3 + i * 2;
        if (x_mult == 0) {
            start_x = player->entity->pos.x + (y_mult * (i + 1));
            start_y = player->entity->pos.y + (y_mult * (i + 1));
        } else {
            start_x = player->entity->pos.x + (x_mult * (i + 1));
            start_y = player->entity->pos.y - (x_mult * (i + 1));
        }
        for (int a = 0 ; a < current_view_size ; a++) {
            int new_x = start_x - (a * y_mult);
            int new_y = start_y + (a * x_mult);
                ressources = realloc(ressources, sizeof(char) * (strlen(ressources) + 2));
                strcat(ressources, ",");
            if (x_mult == 0) {
                ressources = get_player_in_tile(game, ressources, normalize(new_x, game->map->size.x), normalize(start_y, game->map->size.y));
                ressources = get_egg_in_tile(game, ressources, normalize(new_x, game->map->size.x), normalize(start_y, game->map->size.y));
                ressources = get_ressource_name(game, ressources, normalize(new_x, game->map->size.x), normalize(start_y, game->map->size.y));
            } else {
                ressources = get_player_in_tile(game, ressources, normalize(start_x, game->map->size.x), normalize(new_y, game->map->size.y));
                ressources = get_ressource_name(game, ressources, normalize(start_x, game->map->size.x), normalize(new_y, game->map->size.y));
            }
            ressources = realloc(ressources, sizeof(char) * (strlen(ressources) + 2));
        }
    }
    ressources = realloc(ressources, sizeof(char) * (strlen(ressources) + 3));
    strcat(ressources, "]\n");
    return ressources;
}

const char *eject_player(game_t *game, player_t *player, const char *arg)
{
    int x = player->entity->pos.x;
    int y = player->entity->pos.y;
    int success = 0;
    for (list_t ptr = game->players ; ptr != NULL ; ptr = ptr->next) {
        player_t *player2 = ptr->value;
        if (strcmp(player2->team_name, "GRAPHIC") == 0)
            continue;
        if (player2->entity->pos.x == x && player2->entity->pos.y == y && player2 != player) {
            player2->entity->pos.x = normalize(player2->entity->pos.x + 1, game->map->size.x);
            player2->entity->pos.y = normalize(player2->entity->pos.y + 1, game->map->size.y);
            dprintf(player2->fd, "eject: %d\n", get_from_orientation(player));
            success = 1;
        }
    }
    destroy_egg(game, player->entity->pos.x, player->entity->pos.y, &success);
    if (success == 1)
        return "ok\n";
    return "ko\n";
}

const char *fork_player(game_t *game, player_t *player, const char *arg)
{
    team_t *team = get_team_by_name(game, player->team_name);
    add_egg(game, team->name);
    team->max_players ++;
    //gui communication
    gui_enw(game, player, ((egg_t *) list_get_elem_at_back(game->eggs))->id);
    gui_send_all(game, game->send_message);
    return "ok\n";
}

void send_broadcast_message(const char *message, int dest_fd, int tile_from)
{
    dprintf(dest_fd, "message %d, %s\n", tile_from, message);
}

int convert_provenance(int provenance, pos_t orientation)
{
    if (provenance == 0)
        return 0;
    int to_add = 0;
    if (orientation.x == 1) {
        to_add = 2;
    }
    if (orientation.x == -1) {
        to_add = 6;
    }
    if (orientation.y == 1) {
        to_add = 4;
    }
    int new_provenance = (provenance + to_add) % 9;
    if (new_provenance == 0)
        new_provenance = 1;
    return new_provenance;
}

pos_t check_better_pos(pos_t curr, pos_t new, pos_t sender)
{
    int total_curr = abs(curr.x - sender.x) + abs(curr.y - sender.y);
    int total_new = abs(new.x - sender.x) + abs(new.y - sender.y);
    if (total_curr < total_new)
        return curr;
    return new;
}

int get_impact_point(pos_t receiver, pos_t sender)
{
    if (receiver.y > sender.y) {
        if (receiver.x < sender.x)
            return 8;
        else if (receiver.x > sender.x)
            return 2;
        else
            return 1;
    }
    if (receiver.y == sender.y) {
        if (receiver.x < sender.x)
            return 7;
        else if (receiver.x > sender.x)
            return 3;
        else
            return 0;
    }
    if (receiver.y < sender.y) {
        if (receiver.x < sender.x)
            return 6;
        else if (receiver.x > sender.x)
            return 4;
        else
            return 5;
    }
    return -1;
}

int find_provenance(game_t *game, player_t *sender, player_t *receiver)
{
    pos_t base = receiver->entity->pos;
    pos_t max = game->map->size;
    pos_t all_pos[5] = {
        {base.x, base.y},
        {base.x, base.y - max.y},
        {base.x, base.y + max.y},
        {base.x - max.x, base.y},
        {base.x + max.x, base.y}};
    pos_t better = {base.x, base.y};
    for (int i = 1 ; i < 5 ; i++) {
        pos_t buff = {all_pos[i].x, all_pos[i].y};
        better = check_better_pos(better, buff, sender->entity->pos);
    }
    return convert_provenance(get_impact_point(better, sender->entity->pos), receiver->entity->orientation);
}

const char *broadcast(game_t *game, player_t *player, const char *arg)
{
    for (list_t ptr = game->players ; ptr != NULL ; ptr = ptr->next) {
        player_t *player2 = ptr->value;
        if (strcmp(player2->team_name, "GRAPHIC") == 0 || player2->fd == player->fd)
            continue;
        send_broadcast_message(arg, player2->fd, find_provenance(game, player, player2));
    }
    //gui communication
    gui_pbc(game, player->fd, arg);
    gui_send_all(game, game->send_message);
    return "ok\n";
}
