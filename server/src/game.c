/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Game logic implementation
*/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "game.h"
#include "utils.h"
#include "resources.h"

game_t *game_create(int width, int height, char **team_names, int team_count, int clients_nb)
{
    printf("DEBUG: Starting game_create with %dx%d map, %d teams, %d clients per team\n", 
           width, height, team_count, clients_nb);
    fflush(stdout);
    
    // Validate parameters
    if (width <= 0 || height <= 0 || team_count <= 0 || clients_nb <= 0 || !team_names) {
        printf("ERROR: Invalid game parameters\n");
        return NULL;
    }
    
    game_t *game = calloc(1, sizeof(game_t));
    if (!game) {
        printf("ERROR: Failed to allocate game\n");
        return NULL;
    }
    printf("DEBUG: Game structure allocated\n");
    fflush(stdout);

    // Create map
    printf("DEBUG: Creating map %dx%d\n", width, height);
    fflush(stdout);
    game->map = map_create(width, height);
    if (!game->map) {
        printf("ERROR: Failed to create map\n");
        free(game);
        return NULL;
    }
    printf("DEBUG: Map created successfully\n");
    fflush(stdout);

    // Create teams
    printf("DEBUG: Creating %d teams\n", team_count);
    fflush(stdout);
    game->teams = calloc(team_count, sizeof(team_t *));
    if (!game->teams) {
        printf("ERROR: Failed to allocate teams array\n");
        map_destroy(game->map);
        free(game);
        return NULL;
    }
    game->team_count = team_count;
    
    for (int i = 0; i < team_count; i++) {
        printf("DEBUG: Creating team %d: %s\n", i, team_names[i]);
        fflush(stdout);
        game->teams[i] = team_create(i, team_names[i], clients_nb);
        if (!game->teams[i]) {
            printf("ERROR: Failed to create team %d\n", i);
            // Cleanup already created teams
            for (int j = 0; j < i; j++) {
                team_destroy(game->teams[j]);
            }
            free(game->teams);
            map_destroy(game->map);
            free(game);
            return NULL;
        }
        
        // Create initial eggs
        printf("DEBUG: Creating %d initial eggs for team %s\n", clients_nb, team_names[i]);
        fflush(stdout);
        for (int j = 0; j < clients_nb; j++) {
            int x = rand() % width;
            int y = rand() % height;
            egg_t *egg = team_add_egg(game->teams[i], game->next_egg_id++, x, y);
            if (egg) {
                map_add_egg(game->map, x, y, egg->id);
                printf("DEBUG: Egg %d created at (%d,%d) for team %s\n", 
                       egg->id, x, y, team_names[i]);
            } else {
                printf("WARNING: Failed to create egg for team %s\n", team_names[i]);
            }
        }
    }
    printf("DEBUG: All teams created successfully\n");
    fflush(stdout);

    // Initialize players array
    game->player_capacity = 16;
    game->players = calloc(game->player_capacity, sizeof(player_t *));
    if (!game->players) {
        printf("ERROR: Failed to allocate players array\n");
        game_destroy(game);
        return NULL;
    }
    
    // Initialize IDs
    game->next_player_id = 1;
    game->next_egg_id = team_count * clients_nb + 1;
    
    // Spawn initial resources
    printf("DEBUG: Spawning initial resources\n");
    fflush(stdout);
    game_spawn_resources(game);

    srand(time(NULL));
    
    printf("DEBUG: Game creation completed successfully\n");
    fflush(stdout);
    return game;
}

void game_destroy(game_t *game)
{
    if (!game) return;

    // Destroy map
    if (game->map) map_destroy(game->map);

    // Destroy teams
    for (int i = 0; i < game->team_count; i++) {
        if (game->teams[i]) team_destroy(game->teams[i]);
    }
    free(game->teams);

    // Destroy players
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i]) player_destroy(game->players[i]);
    }
    free(game->players);

    if (game->winning_team) free(game->winning_team);

    free(game);
}

player_t *game_add_player(game_t *game, int client_id, const char *team_name)
{
    // Find team
    team_t *team = game_get_team_by_name(game, team_name);
    if (!team || team->egg_count == 0) return NULL;

    // Get random egg
    egg_t *egg = team_get_random_egg(team);
    if (!egg) return NULL;

    // Create player at egg position
    player_t *player = player_create(game->next_player_id++, client_id, 
                                    team->id, egg->x, egg->y);
    if (!player) return NULL;

    // Expand player array if needed
    if (game->player_count >= game->player_capacity) {
        game->player_capacity *= 2;
        game->players = realloc(game->players, 
                               game->player_capacity * sizeof(player_t *));
    }

    // Add player
    game->players[game->player_count++] = player;
    map_add_player(game->map, player->x, player->y, player->id);

    // Remove egg
    map_remove_egg(game->map, egg->x, egg->y, egg->id);
    team_remove_egg(team, egg->id);
    team->connected_clients++;

    return player;
}

void game_remove_player(game_t *game, int player_id)
{
    // Find player
    int index = -1;
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i]->id == player_id) {
            index = i;
            break;
        }
    }

    if (index < 0) return;

    player_t *player = game->players[index];
    
    // Remove from map
    map_remove_player(game->map, player->x, player->y, player->id);
    
    // Update team
    if (player->team_id >= 0 && player->team_id < game->team_count) {
        game->teams[player->team_id]->connected_clients--;
    }
    
    // Drop inventory
    tile_t *tile = map_get_tile(game->map, player->x, player->y);
    for (int i = 0; i < RESOURCE_COUNT; i++) {
        tile->resources[i] += player->inventory[i];
    }
    
    // Destroy player
    player_destroy(player);
    
    // Remove from array
    for (int i = index; i < game->player_count - 1; i++) {
        game->players[i] = game->players[i + 1];
    }
    game->player_count--;
}

team_t *game_get_team_by_name(game_t *game, const char *name)
{
    for (int i = 0; i < game->team_count; i++) {
        if (strcmp(game->teams[i]->name, name) == 0) {
            return game->teams[i];
        }
    }
    return NULL;
}

player_t *game_get_player_by_id(game_t *game, int player_id)
{
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i]->id == player_id) {
            return game->players[i];
        }
    }
    return NULL;
}

player_t *game_get_player_by_client_id(game_t *game, int client_id)
{
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i]->client_id == client_id) {
            return game->players[i];
        }
    }
    return NULL;
}

void game_tick(game_t *game, int freq)
{
    // Update all players
    for (int i = 0; i < game->player_count; i++) {
        player_t *player = game->players[i];
        
        // Consume life
        player_consume_life(player);
    }
    
    // Remove dead players
    for (int i = game->player_count - 1; i >= 0; i--) {
        if (game->players[i]->is_dead) {
            log_info("Player %d died", game->players[i]->id);
            game_remove_player(game, game->players[i]->id);
        }
    }
    
    // Spawn resources every 20 time units
    game->resource_timer++;
    if (game->resource_timer >= 20) {
        game->resource_timer = 0;
        game_spawn_resources(game);
        log_info("Resources spawned");
    }
}

bool game_check_victory(game_t *game)
{
    // Check each team
    for (int i = 0; i < game->team_count; i++) {
        int level8_count = 0;
        
        // Count level 8 players
        for (int j = 0; j < game->player_count; j++) {
            if (game->players[j]->team_id == i && 
                game->players[j]->level >= 8) {
                level8_count++;
            }
        }
        
        // Victory condition: 6 players at level 8
        if (level8_count >= 6) {
            game->game_won = true;
            game->winning_team = strdup(game->teams[i]->name);
            return true;
        }
    }
    
    return false;
}

void game_spawn_resources(game_t *game)
{
    int total_tiles = game->map->width * game->map->height;
    
    for (int res = 0; res < RESOURCE_COUNT; res++) {
        int target = (int)(total_tiles * RESOURCE_DENSITY[res]);
        int current = 0;
        
        // Count current resources
        for (int y = 0; y < game->map->height; y++) {
            for (int x = 0; x < game->map->width; x++) {
                tile_t *tile = map_get_tile(game->map, x, y);
                current += tile->resources[res];
            }
        }
        
        // Spawn missing resources
        int to_spawn = target - current;
        while (to_spawn > 0) {
            int x = rand() % game->map->width;
            int y = rand() % game->map->height;
            tile_t *tile = map_get_tile(game->map, x, y);
            tile->resources[res]++;
            to_spawn--;
        }
    }
}