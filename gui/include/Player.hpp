#pragma once

#include "raylib.h"
#include <vector>
#include <string>
#include "ResourceManager.hpp"
#include <iostream>

class Player {
public:
    Player(int id, int teamId);
    ~Player() = default;

    // Update
    void SetPosition(int x, int y);
    void SetOrientation(int orientation);
    void SetLevel(int level);
    void SetInventory(const std::vector<int>& inventory);
    void SetTileSize(float tileSize) { m_tileSize = tileSize; }
    void SetMapSize(int width, int height) { m_mapWidth = width; m_mapHeight = height; }
    void SetServerTickRate(float tickRate) { m_serverTickRate = tickRate; }
    void Look();  // Fait tourner le joueur sur lui-même
    void StartCommand(const std::string& command);  // Démarrer une commande avec son temps
    void SpawnAnimation();  // Animation de spawn pour les nouveaux crabes
    void Update(float deltaTime);

    // Rendering
    void Draw(Model& model, Color teamColor, bool showBoundingBox = false);
    BoundingBox GetBoundingBox() const;

    // Getters
    int GetId() const { return m_id; }
    int GetTeamId() const { return m_teamId; }
    int GetX() const { return m_x; }
    int GetY() const { return m_y; }
    int GetLevel() const { return m_level; }
    const std::vector<int>& GetInventory() const { return m_inventory; }
    Vector3 GetWorldPosition() const { return m_worldPosition; }
    enum class AnimState { IDLE, RUN, CAST, INCANT, CELEBRATE, INTERACT };
    void SetAnimState(AnimState state, float specialDuration = 0.0f);
    AnimState GetAnimState() const { return m_animState; }
    void SetResourceManager(ResourceManager* res) { 
        m_res = res; 
        std::cout << "Player " << m_id << " ResourceManager assigned: " << (m_res ? "OK" : "NULL") << std::endl;
        if (m_res) {
            SetAnimState(AnimState::IDLE);  // Initialize default animation
        }
    }

private:
    int m_id;
    int m_teamId;
    int m_x, m_y;
    int m_orientation;
    int m_level;
    std::vector<int> m_inventory;
    
    // 3D properties
    Vector3 m_worldPosition;
    Vector3 m_targetPosition;
    float m_rotation;
    float m_targetRotation;
    float m_moveSpeed = 5.0f;
    
    // Animation
    float m_animationTime = 0.0f;
    bool m_isMoving = false;
    AnimState m_animState = AnimState::IDLE;
    int m_currentAnim = 0;
    float m_animFrame = 0.0f;
    ResourceManager* m_res = nullptr;
    float m_tileSize = 2.0f; // Taille des tiles pour ce joueur
    float m_specialAnimTimer = 0.0f;
    int m_mapWidth = 10;  // Taille de la map par défaut
    int m_mapHeight = 10; // Taille de la map par défaut
    float m_serverTickRate = 10.0f;  // Ticks par seconde par défaut
    
    // Command timing
    std::string m_currentCommand = "";
    float m_commandTimer = 0.0f;
    float m_commandDuration = 0.0f;
    bool m_isExecutingCommand = false;
}; 