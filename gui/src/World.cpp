#include "World.hpp"
#include "ResourceManager.hpp"
#include "rlgl.h"
#include <iostream>

World::World(ResourceManager* resources) : m_resources(resources) {
}

void World::Initialize(int width, int height, float tileSize) {
    m_width = width;
    m_height = height;
    m_tileSize = tileSize;
    
    m_tiles.clear();
    m_tiles.reserve(width * height);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            m_tiles.push_back(std::make_unique<Tile>(x, y, tileSize));
        }
    }
}

void World::Clear() {
    m_tiles.clear();
    m_players.clear();
}

void World::UpdateTile(int x, int y, const TileData& data) {
    int idx = GetTileIndex(x, y);
    if (idx >= 0 && idx < static_cast<int>(m_tiles.size())) {
        m_tiles[idx]->SetData(data);
    }
}

void World::AddPlayer(int id, int teamId, int x, int y, 
                     int orientation, int level) {
    auto player = std::make_unique<Player>(id, teamId);
    player->SetTileSize(m_tileSize);
    player->SetMapSize(m_width, m_height);
    player->SetPosition(x, y);
    player->SetOrientation(orientation);
    player->SetLevel(level);
    player->SetResourceManager(m_resources);
    player->SpawnAnimation();
    m_players[id] = std::move(player);
}

void World::UpdatePlayer(int id, int x, int y, int orientation) {
    auto it = m_players.find(id);
    if (it != m_players.end()) {
        it->second->SetMapSize(m_width, m_height);
        it->second->SetPosition(x, y);
        it->second->SetOrientation(orientation);
    }
}

void World::RemovePlayer(int id) {
    m_players.erase(id);
}

void World::Draw(bool showBoundingBoxes, const Camera3D& camera) {
    
    // Draw tiles
    if (!m_tiles.empty()) {
        Model& tileModel = m_resources->GetTileModel();
        Texture2D& grassTexture = m_resources->GetTexture("grass");
        Model resourceModels[7];
        Color resourceColors[7];
        for (int i = 0; i < 7; ++i) {
            resourceModels[i] = m_resources->GetResourceModel(i);
            resourceColors[i] = m_resources->GetResourceColor(i);
        }
        for (auto& tile : m_tiles) {
            tile->Draw(tileModel, grassTexture);
            tile->DrawResources(resourceModels, resourceColors);
        }
    }
    
    // Draw players with correct model per team
    for (auto& [id, player] : m_players) {
        int teamId = player->GetTeamId();
        Model& playerModel = m_resources->GetPlayerModel(teamId);
        Color teamColor = m_resources->GetTeamColor(teamId);
        player->Draw(playerModel, teamColor, showBoundingBoxes);
        
    }
}

int World::GetPlayerAt(Ray ray) {
    for (auto& [id, player] : m_players) {
        BoundingBox box = player->GetBoundingBox();
        RayCollision collision = GetRayCollisionBox(ray, box);
        if (collision.hit) {
            return id;
        }
    }
    return -1;
}

Vector3 World::GetWorldPosition(int x, int y) {
    return { x * m_tileSize, 0, y * m_tileSize };
}

int World::GetTileIndex(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return -1;
    }
    return y * m_width + x;
}

Player* World::GetPlayer(int id) {
    auto it = m_players.find(id);
    return (it != m_players.end()) ? it->second.get() : nullptr;
}

void World::UpdatePlayerInventory(int id, const std::vector<int>& inventory) {
    auto it = m_players.find(id);
    if (it != m_players.end()) {
        it->second->SetInventory(inventory);
    }
}

void World::PlayerLook(int id) {
    auto it = m_players.find(id);
    if (it != m_players.end()) {
        it->second->Look();
    }
}

void World::StartPlayerCommand(int id, const std::string& command) {
    auto it = m_players.find(id);
    if (it != m_players.end()) {
        it->second->StartCommand(command);
    }
}

void World::SetServerTickRate(float tickRate) {
    for (auto& [id, player] : m_players) {
        player->SetServerTickRate(tickRate);
    }
}

void World::Update(float deltaTime) {
    for (auto& [id, player] : m_players) {
        player->Update(deltaTime);
    }
} 