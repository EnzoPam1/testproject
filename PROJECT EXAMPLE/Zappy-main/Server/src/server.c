#include "server.h"
#include "actions.h"

/**
 * Initializes the server structure
 */
server_t *server_new(int argc, char *argv[])
{
    server_t *server = calloc(1, sizeof(server_t));
    server->options = options_new(argc, argv);
    server->netctl = netctl_new(server->options->port);
    server->game = init_game(server->options);
    return server;
}

/**
 * Selects the sockets that are ready to be read
 * handle new connection request and read data from clients
 */
void server_select(server_t *server)
{
    static int remaining_nsec = 0;
    size_t elapsed, dt;
    int act;
    fd_set readfds;
    timeval_t *timeout;
    static struct timespec start = {0}, end = {0};

    readfds = server->netctl->watched_fd;

    timeout = server_get_next_timeout(server);
    if (start.tv_sec == 0 && start.tv_nsec == 0)
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    act = select(FD_SETSIZE, &readfds, NULL, NULL, timeout);

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    free(timeout);

    dt = (end.tv_nsec - start.tv_nsec) + ((end.tv_sec - start.tv_sec) * 1E9);
    elapsed = dt * server->game->freq * 1E-9;
    remaining_nsec += (dt * server->game->freq) % (int)1E9;

    if (remaining_nsec >= (int)1E9)
    {
        elapsed += remaining_nsec / (int)1E9;
        remaining_nsec %= (int)1E9;
    }

    start = end;

    actions_apply_elapsed_time(server->actions, elapsed);
    player_decrease_food(server->game->players, elapsed);
    server->game->ressources_time_unit -= elapsed;

    server_process_activity(server, &readfds, act);
}

/**
 * Sends the welcome message to the client and waits for the team name
 */
bool server_handshake(server_t *server, socket_t *s)
{
    s->handshaked = true;
    if (strncmp(s->buffer, "GRAPHIC", 7) == 0)
    {
        add_player(server->game, "GRAPHIC", s->fd);
        gui_send_at_connexion(server->game, s->fd);
        return true;
    }
    for (list_t head = server->game->teams; head != NULL; head = head->next)
    {
        team_t *team = (team_t *)head->value;
        if (strncmp(team->name, s->buffer, strlen(team->name)) == 0 && team->nb_players < team->max_players)
        {
            dprintf(s->fd, "%d\n"
                           "%d %d\n",
                    team->max_players - team->nb_players,
                    server->options->width,
                    server->options->height);
            add_player(server->game, team->name, s->fd);
            gui_player_connexion(server->game, get_player_by_fd(server->game, s->fd));
            gui_send_all(server->game, server->game->send_message);
            return true;
        }
    }
    dprintf(s->fd, "0\n"
                   "0 0\n");
    remove_player(server->game, s->fd, false);
    netctl_disconnect(server->netctl, s->fd);
    return false;
}

/**
 * Server main loop
 */
int server_run(server_t *server)
{
    while (1)
    {
        server_select(server);
        for (list_t head = server->game->players; head != NULL; head = head->next)
        {
            player_t *player = (player_t *)head->value;
            if (strncmp(player->team_name, "GRAPHIC", 7) == 0)
                continue;
            if (player->entity->food_timer_units <= 0)
            {
                player->entity->food_left -= 1;
                player->entity->food_timer_units += 126;
                gui_player_inventory(server->game, player);
                gui_send_all(server->game, server->game->send_message);
            }
        }
        for (list_t *head = &server->game->players; (*head) != NULL;)
        {
            player_t *player = (player_t *)(*head)->value;
            if (strncmp(player->team_name, "GRAPHIC", 7) == 0)
            {
                head = &(*head)->next;
                continue;
            }
            if (player->entity->food_left <= 0)
            {
                actions_remove_from_issuer(&server->actions, player->fd);
                remove_player(server->game, player->fd, true);
                netctl_disconnect(server->netctl, player->fd);
                continue;
            }
            head = &(*head)->next;
        }
        list_t *head = &server->actions;
        while (*head != NULL)
        {
            action_t *action = (action_t *)(*head)->value;
            if (action->cooldown <= 0)
            {

                player_t *p = get_player_by_fd(server->game, action->issuer);
                const char *msg = action->callback(server->game, p, action->arg);
                dprintf(action->issuer, msg);
                free(action);
                list_del_elem_at_front(head);
            }
            else
                head = &((*head)->next);
        }
        if (server->game->ressources_time_unit <= 0)
        {
            spawn_ressources(server->game);
            server->game->ressources_time_unit += 20;
        }
    }
    return 0;
}

/**
 * Frees the server structure
 */
void server_destroy(server_t *server)
{
    options_destroy(server->options);
    netctl_destroy(server->netctl);
    free(server);
}

/**
 * Processes the client buffer after recving data
 * for each line if the client is handshaked
 */
bool server_process_buffer(server_t *server, socket_t *s)
{
    player_t *player = get_player_by_fd(server->game, s->fd);
    char *eol;
    while ((eol = strchr(s->buffer, '\n')) != NULL)
    {
        *eol = '\0';
        if (!s->handshaked)
        {
            if (!server_handshake(server, s))
                return false;
        }
        else
        {
            if (strcmp(player->team_name, "GRAPHIC") == 0)
                gui_request_process(server->game, player, s->buffer);
            else
            {
                if (strncmp(s->buffer, "Incantation", 11) == 0)
                {
                    const char *buff = verif_incantation(server->game, player, NULL, 0);
                    if (strncmp(buff, "ko\n", 3) == 0)
                    {
                        dprintf(player->fd, buff);
                        return true;
                    }
                    get_incantation(server->game, player);
                }
                if (!actions_accept(&server->actions, action_new(s->fd, s->buffer)))
                    s->fails_count += 1;
                else
                    s->fails_count = 0;
            }
        }
        s->bufsz -= strlen(s->buffer) + 1;
        memmove(s->buffer, eol + 1, s->bufsz);
    }
    return true;
}

/**
 * Analyzes the activity on the server sockets
 * and processes it
 */
void server_process_activity(server_t *server, fd_set *readfds, int act)
{
    if (FD_ISSET(server->netctl->entrypoint.fd, readfds))
    {
        int fd = netctl_accept(server->netctl);
        send(fd, "WELCOME\n", 8, 0);
    }
    for (list_t head = server->netctl->clients; head != NULL;)
    {
        if (FD_ISSET(((socket_t *)head->value)->fd, readfds))
        {
            char query[1024] = {0};
            socket_t *s = (socket_t *)head->value;
            ssize_t rbytes = recv(((socket_t *)head->value)->fd, query, 1024, 0);
            if ((rbytes == 0 || rbytes == -1) || s->fails_count >= 10)
            {
                if (s->fails_count >= 10)
                    dprintf(s->fd, "kicked for spamming nonsense\n");
                if (FD_ISSET(s->fd, &server->netctl->watched_fd)) {
                    actions_remove_from_issuer(&server->actions, ((socket_t *)head->value)->fd);
                    remove_player(server->game, ((socket_t *)head->value)->fd, false);
                    netctl_disconnect(server->netctl, ((socket_t *)head->value)->fd);
                    head = server->netctl->clients;
                    continue;
                }
            }
            else
            {
                s->bufsz += rbytes;
                s->buffer = realloc(s->buffer, s->bufsz);
                memcpy(s->buffer + s->bufsz - rbytes, query, rbytes);
                if (!server_process_buffer(server, s))
                {
                    printf("Client disconnected\n");
                    head = server->netctl->clients;
                    continue;
                }
            }
        }
        head = head->next;
    }
}