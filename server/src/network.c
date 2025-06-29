/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Network handling - CORRECTED VERSION
*/

#include "network.h"
#include "buffer.h"
#include "commands.h"
#include "utils.h"
#include "client.h"
#include "server.h"
#include "player.h"
#include "actions.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>

int network_setup_listener(server_t *srv)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(srv->port),
        .sin_addr = {.s_addr = INADDR_ANY}
    };

    if (fd < 0)
        return -1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        return -1;
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, 10) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void send_response(int fd, const char *msg)
{
    char formatted[1024];
    int len;
    
    // Ensure response has proper line termination
    if (msg[strlen(msg) - 1] != '\n') {
        len = snprintf(formatted, sizeof(formatted), "%s\n", msg);
    } else {
        len = snprintf(formatted, sizeof(formatted), "%s", msg);
    }
    
    if (len > 0 && len < sizeof(formatted)) {
        send(fd, formatted, len, 0);
        log_info("SEND[%d]: %.*s", fd, len - 1, formatted); // Log without \n
    }
}

static void sendf(int fd, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    
    // Ensure proper line termination
    if (len > 0 && len < sizeof(buf) - 1 && buf[len - 1] != '\n') {
        buf[len] = '\n';
        buf[len + 1] = '\0';
        len++;
    }
    
    if (len > 0) {
        send(fd, buf, len, 0);
        log_info("SENDF[%d]: %.*s", fd, len - 1, buf); // Log without \n
    }
}

static void remove_client(server_t *srv, int idx)
{
    client_t *cl = srv->clients[idx];

    log_info("Removing client %d (fd=%d)", cl->id, cl->socket_fd);

    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->client_idx == cl->id) {
            char msg[256];
            snprintf(msg, sizeof(msg), "pdi #%d\n", srv->players[i]->id);
            broadcast_to_gui(srv, msg);

            player_destroy(srv->players[i]);
            for (int j = i + 1; j < srv->player_count; j++)
                srv->players[j - 1] = srv->players[j];
            srv->player_count--;
            break;
        }
    }

    close(cl->socket_fd);
    client_destroy(cl);
    for (int j = idx + 1; j < srv->client_count; j++)
        srv->clients[j - 1] = srv->clients[j];
    srv->client_count--;
}

static void process_client_data(server_t *srv, client_t *cl, const char *data, ssize_t len)
{
    // Add all received data to buffer first
    buffer_write(&cl->buf, data, len);
    
    // Process complete lines from buffer
    char line[BUF_SIZE];
    while (buffer_readline(&cl->buf, line, sizeof(line))) {
        // Remove any trailing \r or \n
        size_t line_len = strlen(line);
        while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r')) {
            line[line_len - 1] = '\0';
            line_len--;
        }
        
        if (line_len > 0) {
            log_info("RECV[%d]: '%s'", cl->socket_fd, line);
            
            if (cl->state == STATE_AUTH) {
                int handled = 0;
                for (int j = 0; j < srv->teams_count; j++) {
                    if (strcmp(line, srv->team_names[j]) == 0) {
                        if (srv->slots_remaining[j] > 0) {
                            cl->is_gui = 0;
                            cl->team_idx = j;
                            cl->state = STATE_ACTIVE;
                            srv->slots_remaining[j]--;

                            sendf(cl->socket_fd, "%d", srv->slots_remaining[j]);
                            sendf(cl->socket_fd, "%d %d", srv->width, srv->height);
                            
                            int spawn_x = rand() % srv->width;
                            int spawn_y = rand() % srv->height;
                            player_t *player = player_create(cl->id, j, spawn_x, spawn_y);

                            if (srv->player_count == srv->player_capacity) {
                                int newcap = srv->player_capacity * 2;
                                player_t **tmp = realloc(srv->players, sizeof(*tmp) * newcap);
                                if (!tmp)
                                    die("realloc players");
                                srv->players = tmp;
                                srv->player_capacity = newcap;
                            }
                            srv->players[srv->player_count++] = player;

                            char msg[256];
                            snprintf(msg, sizeof(msg),
                                "pnw #%d %d %d %d %d %s\n",
                                player->id, player->x, player->y,
                                player->orientation, player->level,
                                srv->team_names[j]);
                            broadcast_to_gui(srv, msg);
                            
                            log_info("Player #%d created for team %s at (%d,%d)", 
                                   player->id, srv->team_names[j], spawn_x, spawn_y);
                        } else {
                            sendf(cl->socket_fd, "0");
                            for (int k = 0; k < srv->client_count; k++) {
                                if (srv->clients[k] == cl) {
                                    remove_client(srv, k);
                                    return;
                                }
                            }
                        }
                        handled = 1;
                        break;
                    }
                }
                if (!handled && strcmp(line, "GRAPHIC") == 0) {
                    cl->is_gui = 1;
                    cl->state = STATE_ACTIVE;
                    handled = 1;

                    sendf(cl->socket_fd, "msz %d %d", srv->width, srv->height);
                    for (int y = 0; y < srv->height; y++) {
                        for (int x = 0; x < srv->width; x++) {
                            tile_t *tile = &srv->map[y][x];
                            sendf(cl->socket_fd, "bct %d %d %d %d %d %d %d %d %d",
                                x, y, tile->food, tile->stones[0], tile->stones[1],
                                tile->stones[2], tile->stones[3], tile->stones[4],
                                tile->stones[5]);
                        }
                    }
                    for (int j = 0; j < srv->teams_count; j++)
                        sendf(cl->socket_fd, "tna %s", srv->team_names[j]);

                    for (int j = 0; j < srv->player_count; j++) {
                        player_t *p = srv->players[j];
                        sendf(cl->socket_fd, "pnw #%d %d %d %d %d %s",
                            p->id, p->x, p->y, p->orientation, p->level,
                            srv->team_names[p->team_idx]);
                    }

                    sendf(cl->socket_fd, "sgt %d", srv->freq);
                }
                if (!handled) {
                    log_info("Unknown team name '%s' from client %d", line, cl->socket_fd);
                    sendf(cl->socket_fd, "ko");
                    for (int k = 0; k < srv->client_count; k++) {
                        if (srv->clients[k] == cl) {
                            remove_client(srv, k);
                            return;
                        }
                    }
                }
            } else {
                dispatch_command(srv, cl, line);
            }
        }
    }
}

void network_handle_new_connection(server_t *srv)
{
    struct sockaddr_in peer;
    socklen_t len = sizeof(peer);
    int fd = accept(srv->listen_fd, (struct sockaddr*)&peer, &len);
    static int next_client_id = 1;

    if (fd < 0)
        return;

    // Set socket to non-blocking mode
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    log_info("New client connected (fd=%d, id=%d)", fd, next_client_id);
    client_t *cl = client_create(fd);
    buffer_init(&cl->buf);
    cl->state = STATE_AUTH;
    cl->id = next_client_id++;

    if (srv->client_count == srv->client_capacity) {
        int newcap = srv->client_capacity * 2;
        client_t **tmp = realloc(srv->clients, sizeof(*tmp) * newcap);
        if (!tmp)
            die("realloc clients");
        srv->clients = tmp;
        srv->client_capacity = newcap;
    }
    srv->clients[srv->client_count++] = cl;

    send_response(fd, "WELCOME");
}

void network_handle_client_io(server_t *srv, client_t *cl)
{
    char tmp[512];
    ssize_t r = recv(cl->socket_fd, tmp, sizeof(tmp), 0);

    if (r <= 0) {
        if (r == 0) {
            log_info("Client %d disconnected normally", cl->socket_fd);
        } else {
            log_info("Client %d disconnected with error", cl->socket_fd);
        }
        
        for (int i = 0; i < srv->client_count; i++) {
            if (srv->clients[i] == cl) {
                remove_client(srv, i);
                break;
            }
        }
        return;
    }

    log_info("RAW[%d]: received %zd bytes: '%.*s'", cl->socket_fd, r, (int)r, tmp);
    
    // Add all data to buffer
    buffer_write(&cl->buf, tmp, r);
    
    // Process all complete lines
    char line[BUF_SIZE];
    while (buffer_readline(&cl->buf, line, sizeof(line))) {
        // Remove any trailing whitespace
        char *end = line + strlen(line) - 1;
        while (end >= line && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }
        
        if (strlen(line) > 0) {
            log_info("PROCESSING[%d]: '%s'", cl->socket_fd, line);
            
            if (cl->state == STATE_AUTH) {
                int handled = 0;
                
                // Check for team names
                for (int j = 0; j < srv->teams_count; j++) {
                    if (strcmp(line, srv->team_names[j]) == 0) {
                        if (srv->slots_remaining[j] > 0) {
                            log_info("Client %d authenticated as team '%s'", cl->socket_fd, line);
                            cl->is_gui = 0;
                            cl->team_idx = j;
                            cl->state = STATE_ACTIVE;
                            srv->slots_remaining[j]--;

                            // Send remaining slots
                            sendf(cl->socket_fd, "%d", srv->slots_remaining[j]);
                            // Send world dimensions  
                            sendf(cl->socket_fd, "%d %d", srv->width, srv->height);
                            
                            // Create player
                            int spawn_x = rand() % srv->width;
                            int spawn_y = rand() % srv->height;
                            player_t *player = player_create(cl->id, j, spawn_x, spawn_y);

                            if (srv->player_count == srv->player_capacity) {
                                int newcap = srv->player_capacity * 2;
                                player_t **tmp = realloc(srv->players, sizeof(*tmp) * newcap);
                                if (!tmp)
                                    die("realloc players");
                                srv->players = tmp;
                                srv->player_capacity = newcap;
                            }
                            srv->players[srv->player_count++] = player;

                            // Broadcast to GUI
                            char msg[256];
                            snprintf(msg, sizeof(msg),
                                "pnw #%d %d %d %d %d %s\n",
                                player->id, player->x, player->y,
                                player->orientation, player->level,
                                srv->team_names[j]);
                            broadcast_to_gui(srv, msg);
                            
                            log_info("Player #%d created for team %s at (%d,%d)", 
                                   player->id, srv->team_names[j], spawn_x, spawn_y);
                        } else {
                            log_info("No slots remaining for team '%s'", line);
                            sendf(cl->socket_fd, "0");
                            for (int k = 0; k < srv->client_count; k++) {
                                if (srv->clients[k] == cl) {
                                    remove_client(srv, k);
                                    return;
                                }
                            }
                        }
                        handled = 1;
                        break;
                    }
                }
                
                // Check for GUI
                if (!handled && strcmp(line, "GRAPHIC") == 0) {
                    log_info("Client %d authenticated as GUI", cl->socket_fd);
                    cl->is_gui = 1;
                    cl->state = STATE_ACTIVE;
                    handled = 1;

                    sendf(cl->socket_fd, "msz %d %d", srv->width, srv->height);
                    for (int y = 0; y < srv->height; y++) {
                        for (int x = 0; x < srv->width; x++) {
                            tile_t *tile = &srv->map[y][x];
                            sendf(cl->socket_fd, "bct %d %d %d %d %d %d %d %d %d",
                                x, y, tile->food, tile->stones[0], tile->stones[1],
                                tile->stones[2], tile->stones[3], tile->stones[4],
                                tile->stones[5]);
                        }
                    }
                    for (int j = 0; j < srv->teams_count; j++)
                        sendf(cl->socket_fd, "tna %s", srv->team_names[j]);

                    for (int j = 0; j < srv->player_count; j++) {
                        player_t *p = srv->players[j];
                        sendf(cl->socket_fd, "pnw #%d %d %d %d %d %s",
                            p->id, p->x, p->y, p->orientation, p->level,
                            srv->team_names[p->team_idx]);
                    }

                    sendf(cl->socket_fd, "sgt %d", srv->freq);
                }
                
                if (!handled) {
                    log_info("Unknown team name '%s' from client %d", line, cl->socket_fd);
                    sendf(cl->socket_fd, "ko");
                    for (int k = 0; k < srv->client_count; k++) {
                        if (srv->clients[k] == cl) {
                            remove_client(srv, k);
                            return;
                        }
                    }
                }
            } else {
                // Client is authenticated, process game commands
                dispatch_command(srv, cl, line);
            }
        }
    }
}