/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
** World management
*/

#pragma once

#include "raylib.h"
#include "Tile.hpp"
#include "Player.hpp"
#include <vector>
#include <memory>
#include <unordered_map>

class ResourceManager;

class World {
public:
    World(ResourceManager* resources);
    ~World() = default;

    // Initialization
    void Initialize(int width, int height, float tileSize = 2.0f);
    void Clear();

    // Updates from server
    void UpdateTile(int x, int y, const TileData& data);
    void AddPlayer(int id, int teamId, int x, int y, int orientation, int level);
    void UpdatePlayer(int id, int x, int y, int orientation);
    void RemovePlayer(int id);
    void UpdatePlayerInventory(int id, const std::vector<int>& inventory);
    void SetPlayerLevel(int id, int level);
    void PlayerLook(int id);  // Fait tourner un joueur sur lui-même
    void StartPlayerCommand(int id, const std::string& command);  // Démarrer une commande sur un joueur
    void SetServerTickRate(float tickRate);  // Synchroniser tous les joueurs avec le serveur
    void Update(float deltaTime);  // Update all players

    // Rendering
    void Draw(bool showBoundingBoxes = false, const Camera3D& camera = Camera3D{});
    void DrawTransparent();  // For transparent objects

    // Interaction
    int GetPlayerAt(Ray ray);
    Vector3 GetWorldPosition(int x, int y);
    
    // Getters
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    float GetTileSize() const { return m_tileSize; }
    Player* GetPlayer(int id);
    const std::vector<std::unique_ptr<Tile>>& GetTiles() const { return m_tiles; }

    // Effects
    void StartIncantation(int x, int y, const std::vector<int>& playerIds);
    void EndIncantation(int x, int y, bool success);
    void AddBroadcast(int playerId, const std::string& message);

private:
    ResourceManager* m_resources;
    
    // World dimensions
    int m_width = 0;
    int m_height = 0;
    
    // Tiles
    std::vector<std::unique_ptr<Tile>> m_tiles;
    
    // Players
    std::unordered_map<int, std::unique_ptr<Player>> m_players;
    
    // Visual parameters
    float m_tileSize = 8.0f;  // Much bigger tiles to accommodate natural-sized GLB models
    float m_tileHeight = 0.2f;
    
    // Helper methods
    int GetTileIndex(int x, int y) const;
    void DrawGrid();
    void DrawPlayers();
    void DrawResources();
    void DrawEffects();
}; 