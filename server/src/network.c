/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Network handling - Improved version
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
#include <errno.h>
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

    if (fd < 0) {
        log_info("Failed to create socket: %s", strerror(errno));
        return -1;
    }
    
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        log_info("Failed to set SO_REUSEADDR: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    // Set socket to non-blocking mode
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        log_info("Failed to set non-blocking mode: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_info("Failed to bind socket: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    if (listen(fd, 10) < 0) {
        log_info("Failed to listen on socket: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    log_info("Server listening on port %d", srv->port);
    return fd;
}

static void sendf(int fd, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    
    ssize_t sent = send(fd, buf, strlen(buf), MSG_NOSIGNAL);
    if (sent < 0) {
        log_info("Failed to send data: %s", strerror(errno));
    }
}

static void send_gui_initial_state(server_t *srv, client_t *cl)
{
    // Send map size
    sendf(cl->socket_fd, "msz %d %d\n", srv->width, srv->height);
    
    // Send all tile contents
    for (int y = 0; y < srv->height; y++) {
        for (int x = 0; x < srv->width; x++) {
            tile_t *tile = &srv->map[y][x];
            sendf(cl->socket_fd, "bct %d %d %d %d %d %d %d %d %d\n",
                x, y, tile->food, tile->stones[0], tile->stones[1],
                tile->stones[2], tile->stones[3], tile->stones[4],
                tile->stones[5]);
        }
    }
    
    // Send team names
    for (int i = 0; i < srv->teams_count; i++) {
        sendf(cl->socket_fd, "tna %s\n", srv->team_names[i]);
    }

    // Send all player states
    for (int i = 0; i < srv->player_count; i++) {
        player_t *p = srv->players[i];
        if (p->alive) {
            sendf(cl->socket_fd, "pnw #%d %d %d %d %d %s\n",
                p->id, p->x, p->y, p->orientation, p->level,
                srv->team_names[p->team_idx]);
                
            sendf(cl->socket_fd, "pin #%d %d %d %d %d %d %d %d %d\n",
                p->id, p->x, p->y, p->inventory[0], p->inventory[1], 
                p->inventory[2], p->inventory[3], p->inventory[4], 
                p->inventory[5], p->inventory[6]);
                
            sendf(cl->socket_fd, "plv #%d %d\n", p->id, p->level);
        }
    }

    // Send current frequency
    sendf(cl->socket_fd, "sgt %d\n", srv->freq);
}

static void remove_client(server_t *srv, int idx)
{
    client_t *cl = srv->clients[idx];
    
    log_info("Removing client (fd=%d)", cl->socket_fd);

    // Find and handle associated player
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->client_idx == cl->id) {
            player_t *player = srv->players[i];
            
            // Notify GUI of player death
            char msg[256];
            snprintf(msg, sizeof(msg), "pdi #%d\n", player->id);
            broadcast_to_gui(srv, msg);
            
            // Mark player as dead instead of removing immediately
            player->alive = false;
            
            // Increase available slots for the team
            srv->slots_remaining[player->team_idx]++;
            
            log_info("Player #%d disconnected, marked as dead", player->id);
            break;
        }
    }

    // Close and cleanup client
    close(cl->socket_fd);
    client_destroy(cl);
    
    // Shift array elements
    for (int j = idx + 1; j < srv->client_count; j++) {
        srv->clients[j - 1] = srv->clients[j];
    }
    srv->client_count--;
}

void network_handle_new_connection(server_t *srv)
{
    struct sockaddr_in peer;
    socklen_t len = sizeof(peer);
    int fd = accept(srv->listen_fd, (struct sockaddr*)&peer, &len);
    static int next_client_id = 1;

    if (fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            log_info("Accept failed: %s", strerror(errno));
        }
        return;
    }

    // Set new socket to non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    log_info("New client connected (fd=%d) from %s:%d", 
             fd, inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
    
    client_t *cl = client_create(fd);
    buffer_init(&cl->buf);
    cl->state = STATE_AUTH;
    cl->id = next_client_id++;

    // Expand client array if needed
    if (srv->client_count == srv->client_capacity) {
        int newcap = srv->client_capacity * 2;
        client_t **tmp = realloc(srv->clients, sizeof(*tmp) * newcap);
        if (!tmp) {
            log_info("Failed to expand client array");
            close(fd);
            client_destroy(cl);
            return;
        }
        srv->clients = tmp;
        srv->client_capacity = newcap;
    }
    
    srv->clients[srv->client_count++] = cl;

    // Send welcome message
    sendf(fd, "WELCOME\n");
}

static bool handle_team_authentication(server_t *srv, client_t *cl, const char *team_name)
{
    // Check if it's a GUI client
    if (strcmp(team_name, "GRAPHIC") == 0) {
        cl->is_gui = 1;
        cl->state = STATE_ACTIVE;
        
        log_info("GUI client authenticated (fd=%d)", cl->socket_fd);
        send_gui_initial_state(srv, cl);
        return true;
    }
    
    // Check for valid team
    for (int i = 0; i < srv->teams_count; i++) {
        if (strcmp(team_name, srv->team_names[i]) == 0) {
            if (srv->slots_remaining[i] > 0) {
                cl->is_gui = 0;
                cl->team_idx = i;
                cl->state = STATE_ACTIVE;
                srv->slots_remaining[i]--;

                // Send connection info
                sendf(cl->socket_fd, "%d\n%d %d\n", 
                      srv->slots_remaining[i], srv->width, srv->height);
                
                // Create new player
                int spawn_x = rand() % srv->width;
                int spawn_y = rand() % srv->height;
                player_t *player = player_create(cl->id, i, spawn_x, spawn_y);

                // Expand player array if needed
                if (srv->player_count == srv->player_capacity) {
                    int newcap = srv->player_capacity * 2;
                    player_t **tmp = realloc(srv->players, sizeof(*tmp) * newcap);
                    if (!tmp) {
                        log_info("Failed to expand player array");
                        player_destroy(player);
                        return false;
                    }
                    srv->players = tmp;
                    srv->player_capacity = newcap;
                }
                
                srv->players[srv->player_count++] = player;

                // Notify GUI of new player
                char msg[256];
                snprintf(msg, sizeof(msg), "pnw #%d %d %d %d %d %s\n",
                    player->id, player->x, player->y, player->orientation, 
                    player->level, srv->team_names[i]);
                broadcast_to_gui(srv, msg);
                
                // Send initial inventory to GUI
                snprintf(msg, sizeof(msg), "pin #%d %d %d %d %d %d %d %d %d\n",
                    player->id, player->x, player->y, player->inventory[0], 
                    player->inventory[1], player->inventory[2], player->inventory[3], 
                    player->inventory[4], player->inventory[5], player->inventory[6]);
                broadcast_to_gui(srv, msg);

                log_info("AI client authenticated for team '%s' (fd=%d, player #%d)", 
                         team_name, cl->socket_fd, player->id);
                return true;
            } else {
                // No slots available
                sendf(cl->socket_fd, "0\n");
                log_info("No slots available for team '%s' (fd=%d)", team_name, cl->socket_fd);
                return false;
            }
        }
    }
    
    // Unknown team
    sendf(cl->socket_fd, "0\n");
    log_info("Unknown team '%s' (fd=%d)", team_name, cl->socket_fd);
    return false;
}

void network_handle_client_io(server_t *srv, client_t *cl)
{
    char tmp[4096];
    char line[BUF_SIZE];
    
    // Read data from socket
    ssize_t r = recv(cl->socket_fd, tmp, sizeof(tmp) - 1, MSG_DONTWAIT);
    
    if (r <= 0) {
        if (r == 0) {
            log_info("Client disconnected gracefully (fd=%d)", cl->socket_fd);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            log_info("Client read error (fd=%d): %s", cl->socket_fd, strerror(errno));
        } else {
            return; // Would block, try again later
        }
        
        // Find and remove client
        for (int i = 0; i < srv->client_count; i++) {
            if (srv->clients[i] == cl) {
                remove_client(srv, i);
                break;
            }
        }
        return;
    }

    tmp[r] = '\0';
    
    // Write to buffer
    size_t written = buffer_write(&cl->buf, tmp, (size_t)r);
    if (written < (size_t)r) {
        log_info("Buffer overflow for client (fd=%d)", cl->socket_fd);
    }

    // Process complete lines
    while (buffer_readline(&cl->buf, line, sizeof(line))) {
        line[strcspn(line, "\r\n")] = '\0';
        
        if (strlen(line) == 0) {
            continue; // Skip empty lines
        }

        if (cl->state == STATE_AUTH) {
            if (!handle_team_authentication(srv, cl, line)) {
                // Authentication failed, remove client
                for (int i = 0; i < srv->client_count; i++) {
                    if (srv->clients[i] == cl) {
                        remove_client(srv, i);
                        break;
                    }
                }
                return;
            }
        } else if (cl->state == STATE_ACTIVE) {
            // Process game commands
            dispatch_command(srv, cl, line);
        }
    }
}

void network_cleanup_dead_players(server_t *srv)
{
    // Remove dead players from the array
    for (int i = srv->player_count - 1; i >= 0; i--) {
        if (!srv->players[i]->alive) {
            player_destroy(srv->players[i]);
            
            // Shift remaining players
            for (int j = i + 1; j < srv->player_count; j++) {
                srv->players[j - 1] = srv->players[j];
            }
            srv->player_count--;
        }
    }
}

void network_cleanup(server_t *srv)
{
    if (srv->listen_fd >= 0) {
        close(srv->listen_fd);
    }
    
    for (int i = 0; i < srv->client_count; i++) {
        if (srv->clients[i]) {
            close(srv->clients[i]->socket_fd);
            client_destroy(srv->clients[i]);
        }
    }
    
    network_cleanup_dead_players(srv);
}