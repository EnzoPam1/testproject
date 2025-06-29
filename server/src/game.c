/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Game logic implementation - VERSION CORRIGÉE AVEC GESTION AMÉLIORÉE
*/

#include "game.h"
#include "server.h"
#include "player.h"
#include "utils.h"
#include "actions.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Densités des ressources selon les spécifications
static const float RESOURCE_DENSITY[] = {
    0.5,    // food
    0.3,    // linemate
    0.15,   // deraumere
    0.1,    // sibur
    0.1,    // mendiane
    0.08,   // phiras
    0.05    // thystame
};

// Noms des ressources pour le debugging
static const char *RESOURCE_NAMES[] = {
    "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"
};

static void distribute_resources(tile_t **map, int width, int height)
{
    int total_tiles = width * height;

    log_info("Distribution des ressources sur une carte %dx%d (%d cases)", 
             width, height, total_tiles);

    for (int res = 0; res < 7; res++) {
        int target_count = (int)(total_tiles * RESOURCE_DENSITY[res]);
        if (target_count < 1)
            target_count = 1;

        log_info("Distribution de %d %s (densité: %.2f)", 
                 target_count, RESOURCE_NAMES[res], RESOURCE_DENSITY[res]);

        // Distribution aléatoire équitable
        for (int i = 0; i < target_count; i++) {
            int attempts = 0;
            int max_attempts = total_tiles;
            
            while (attempts < max_attempts) {
                int x = rand() % width;
                int y = rand() % height;
                
                // Éviter la surcharge d'une case
                int current_resources = map[y][x].food;
                for (int j = 0; j < 6; j++) {
                    current_resources += map[y][x].stones[j];
                }
                
                // Limite de ressources par case pour une distribution équitable
                if (current_resources < 5) {
                    if (res == 0) {
                        map[y][x].food++;
                    } else {
                        map[y][x].stones[res - 1]++;
                    }
                    break;
                }
                attempts++;
            }
            
            // Si aucune case appropriée trouvée, placer quand même
            if (attempts >= max_attempts) {
                int x = rand() % width;
                int y = rand() % height;
                if (res == 0) {
                    map[y][x].food++;
                } else {
                    map[y][x].stones[res - 1]++;
                }
            }
        }
    }
    
    // Vérification de la distribution
    int totals[7] = {0};
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            totals[0] += map[y][x].food;
            for (int i = 0; i < 6; i++) {
                totals[i + 1] += map[y][x].stones[i];
            }
        }
    }
    
    for (int i = 0; i < 7; i++) {
        log_info("Total %s: %d", RESOURCE_NAMES[i], totals[i]);
    }
}

tile_t **game_map_init(int width, int height, float density[])
{
    tile_t **map = malloc(sizeof(tile_t*) * height);

    if (!map)
        die("malloc map pointers");
    
    for (int y = 0; y < height; y++) {
        map[y] = malloc(sizeof(tile_t) * width);
        if (!map[y])
            die("malloc map row");
        memset(map[y], 0, sizeof(tile_t) * width);
    }
    
    log_info("Carte initialisée (%dx%d), distribution des ressources...", width, height);
    distribute_resources(map, width, height);
    
    return map;
}

void game_map_free(tile_t **map, int height)
{
    if (!map)
        return;
        
    for (int y = 0; y < height; y++) {
        if (map[y])
            free(map[y]);
    }
    free(map);
}

static void respawn_resources(server_t *srv)
{
    static int tick_count = 0;

    tick_count++;
    
    // Respawn toutes les 20 unités de temps selon les spécifications
    if (tick_count >= 20 * srv->freq) {
        tick_count = 0;
        
        log_info("Respawn des ressources (tick %d)", tick_count);
        
        // Calculer les ressources actuelles
        int current_totals[7] = {0};
        for (int y = 0; y < srv->height; y++) {
            for (int x = 0; x < srv->width; x++) {
                current_totals[0] += srv->map[y][x].food;
                for (int i = 0; i < 6; i++) {
                    current_totals[i + 1] += srv->map[y][x].stones[i];
                }
            }
        }
        
        // Ajouter les ressources manquantes
        int total_tiles = srv->width * srv->height;
        for (int res = 0; res < 7; res++) {
            int target_count = (int)(total_tiles * RESOURCE_DENSITY[res]);
            int deficit = target_count - current_totals[res];
            
            if (deficit > 0) {
                log_info("Ajout de %d %s (déficit détecté)", deficit, RESOURCE_NAMES[res]);
                
                for (int i = 0; i < deficit; i++) {
                    int x = rand() % srv->width;
                    int y = rand() % srv->height;
                    
                    if (res == 0) {
                        srv->map[y][x].food++;
                    } else {
                        srv->map[y][x].stones[res - 1]++;
                    }
                }
            }
        }
    }
}

int check_elevation_requirements(server_t *srv, int x, int y, int level)
{
    if (level < 1 || level > 7) {
        log_info("Niveau d'élévation invalide: %d", level);
        return 0;
    }

    tile_t *tile = &srv->map[y][x];
    
    // Exigences d'élévation selon les spécifications
    int req_players[] = {0, 1, 2, 2, 4, 4, 6, 6};
    int req_stones[8][6] = {
        {0, 0, 0, 0, 0, 0},  // Level 0 (unused)
        {1, 0, 0, 0, 0, 0},  // Level 1->2
        {1, 1, 1, 0, 0, 0},  // Level 2->3
        {2, 0, 1, 0, 2, 0},  // Level 3->4
        {1, 1, 2, 0, 1, 0},  // Level 4->5
        {1, 2, 1, 3, 0, 0},  // Level 5->6
        {1, 2, 3, 0, 1, 0},  // Level 6->7
        {2, 2, 2, 2, 2, 1}   // Level 7->8
    };

    // Compter les joueurs du bon niveau sur la case
    int player_count = 0;
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->x == x && srv->players[i]->y == y &&
            srv->players[i]->level == level) {
            player_count++;
        }
    }

    // Vérifier le nombre de joueurs requis
    if (player_count < req_players[level]) {
        log_info("Élévation niveau %d: joueurs insuffisants (%d/%d)", 
                 level, player_count, req_players[level]);
        return 0;
    }

    // Vérifier les pierres requises
    for (int i = 0; i < 6; i++) {
        if (tile->stones[i] < req_stones[level][i]) {
            log_info("Élévation niveau %d: %s insuffisant (%d/%d)", 
                     level, RESOURCE_NAMES[i + 1], tile->stones[i], req_stones[level][i]);
            return 0;
        }
    }
    
    log_info("Élévation niveau %d: conditions remplies (%d joueurs, ressources OK)", 
             level, player_count);
    return 1;
}

void perform_elevation(server_t *srv, int x, int y, int level)
{
    if (level < 1 || level > 7)
        return;

    tile_t *tile = &srv->map[y][x];
    
    // Consommer les ressources
    int req_stones[8][6] = {
        {0, 0, 0, 0, 0, 0},  // Level 0 (unused)
        {1, 0, 0, 0, 0, 0},  // Level 1->2
        {1, 1, 1, 0, 0, 0},  // Level 2->3
        {2, 0, 1, 0, 2, 0},  // Level 3->4
        {1, 1, 2, 0, 1, 0},  // Level 4->5
        {1, 2, 1, 3, 0, 0},  // Level 5->6
        {1, 2, 3, 0, 1, 0},  // Level 6->7
        {2, 2, 2, 2, 2, 1}   // Level 7->8
    };

    log_info("Consommation des ressources pour élévation niveau %d", level);
    for (int i = 0; i < 6; i++) {
        if (req_stones[level][i] > 0) {
            tile->stones[i] -= req_stones[level][i];
            log_info("Consommé %d %s", req_stones[level][i], RESOURCE_NAMES[i + 1]);
        }
    }

    // Élever tous les joueurs participants
    int elevated = 0;
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->x == x && srv->players[i]->y == y &&
            srv->players[i]->level == level) {
            srv->players[i]->level++;
            srv->players[i]->is_incanting = 0;
            elevated++;
            log_info("Joueur #%d élevé au niveau %d",
                srv->players[i]->id, srv->players[i]->level);
        }
    }

    // Notifier la GUI de la fin d'élévation réussie
    char msg[256];
    snprintf(msg, sizeof(msg), "pie %d %d 1\n", x, y);
    broadcast_to_gui(srv, msg);

    // Vérifier les conditions de victoire
    int level8_count = 0;
    for (int i = 0; i < srv->player_count; i++) {
        if (srv->players[i]->level >= 8) {
            level8_count++;
        }
    }

    if (level8_count >= 6) {
        log_info("VICTOIRE! 6 joueurs ont atteint le niveau 8");
        char victory_msg[256];
        // Trouver l'équipe gagnante (première équipe avec un joueur niveau 8)
        for (int i = 0; i < srv->player_count; i++) {
            if (srv->players[i]->level >= 8) {
                snprintf(victory_msg, sizeof(victory_msg), "seg %s\n",
                    srv->team_names[srv->players[i]->team_idx]);
                broadcast_to_gui(srv, victory_msg);
                break;
            }
        }
    }
}

static void check_incantations(server_t *srv)
{
    // Vérifier les incantations en cours
    for (int i = 0; i < srv->player_count; i++) {
        player_t *player = srv->players[i];
        
        if (player->is_incanting && strlen(player->pending_action) > 0) {
            // L'incantation est gérée dans execute_pending_actions
            continue;
        }
    }
}

static void manage_player_life(server_t *srv)
{
    for (int i = 0; i < srv->player_count; i++) {
        player_t *player = srv->players[i];
        
        if (!player->alive)
            continue;
            
        // Consommation de nourriture toutes les 126 unités de temps
        player->life_units--;
        
        if (player->life_units <= 0) {
            if (player->inventory[RES_FOOD] > 0) {
                player->inventory[RES_FOOD]--;
                player->life_units = LIFE_UNIT_DURATION;  // 126 unités
                log_info("Joueur #%d consomme 1 nourriture, reste: %d",
                    player->id, player->inventory[RES_FOOD]);
                    
                // Notifier la GUI du changement d'inventaire
                char msg[256];
                snprintf(msg, sizeof(msg), "pin #%d %d %d %d %d %d %d %d %d %d\n",
                    player->id, player->x, player->y, player->inventory[0],
                    player->inventory[1], player->inventory[2], player->inventory[3],
                    player->inventory[4], player->inventory[5], player->inventory[6]);
                broadcast_to_gui(srv, msg);
            } else {
                // Joueur mort de faim
                log_info("Joueur #%d mort de faim", player->id);
                player->alive = false;
                
                // Notifier la GUI
                char msg[256];
                snprintf(msg, sizeof(msg), "pdi #%d\n", player->id);
                broadcast_to_gui(srv, msg);
                
                // TODO: Nettoyer le joueur mort
            }
        }
    }
}

void game_tick(server_t *srv)
{
    // Gestion de la vie des joueurs
    manage_player_life(srv);
    
    // Exécution des actions en attente
    execute_pending_actions(srv);
    
    // Vérification des incantations
    check_incantations(srv);
    
    // Respawn des ressources
    respawn_resources(srv);
    
    // Statistiques périodiques (toutes les 100 ticks)
    static int stats_counter = 0;
    stats_counter++;
    
    if (stats_counter >= 100) {
        stats_counter = 0;
        
        int alive_players = 0;
        int total_food = 0;
        for (int i = 0; i < srv->player_count; i++) {
            if (srv->players[i]->alive) {
                alive_players++;
                total_food += srv->players[i]->inventory[RES_FOOD];
            }
        }
        
        log_info("Stats: %d joueurs vivants, nourriture totale: %d", 
                 alive_players, total_food);
    }
}