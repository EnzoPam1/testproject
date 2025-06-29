#include "Player.hpp"
#include <raymath.h>
#include <iostream>

Player::Player(int id, int teamId)
    : m_id(id), m_teamId(teamId), m_x(0), m_y(0),
      m_orientation(1), m_level(1), m_inventory(7, 0) {
    m_worldPosition = { 0, 0, 0 };
    m_targetPosition = m_worldPosition;
    m_rotation = 0;
    m_targetRotation = 0;
    m_currentAnim = 0;
    m_animFrame = 0.0f;
    std::cout << "🦀 Player " << m_id << " created at position (0,0)" << std::endl;
}



void Player::SetPosition(int x, int y) {
    // Logique de donut : si on sort d'un côté, on arrive de l'autre
    // Wrapping des coordonnées
    x = ((x % m_mapWidth) + m_mapWidth) % m_mapWidth;
    y = ((y % m_mapHeight) + m_mapHeight) % m_mapHeight;
    
    // Calculer la direction du mouvement en tenant compte du wrapping
    int deltaX = x - m_x;
    int deltaY = y - m_y;
    
    // Corriger le delta pour le chemin le plus court (wrapping)
    if (deltaX > m_mapWidth / 2) {
        deltaX -= m_mapWidth;  // Prendre le chemin par la gauche
    } else if (deltaX < -m_mapWidth / 2) {
        deltaX += m_mapWidth;  // Prendre le chemin par la droite
    }
    
    if (deltaY > m_mapHeight / 2) {
        deltaY -= m_mapHeight;  // Prendre le chemin par le haut
    } else if (deltaY < -m_mapHeight / 2) {
        deltaY += m_mapHeight;  // Prendre le chemin par le bas
    }
    
    // Ajuster l'orientation selon la direction du mouvement
    if (deltaX != 0 || deltaY != 0) {
        if (deltaX > 0) {
            SetOrientation(2);  // East
        } else if (deltaX < 0) {
            SetOrientation(4);  // West
        } else if (deltaY > 0) {
            SetOrientation(3);  // South
        } else if (deltaY < 0) {
            SetOrientation(1);  // North
        }
    }
    
    m_x = x;
    m_y = y;
    // Position au centre de la tuile, avec hauteur ajustée pour le modèle
    m_targetPosition = {
        x * m_tileSize + m_tileSize * 0.5f,  // Centre de la tuile
        0.5f,                                 // Plus haut au-dessus du sol
        y * m_tileSize + m_tileSize * 0.5f   // Centre de la tuile
    };
    m_isMoving = true;
    std::cout << "🎯 Player " << m_id << " position set to (" << x << "," << y << ") -> world pos (" 
              << m_targetPosition.x << "," << m_targetPosition.z << ") direction: " << m_orientation 
              << " (delta: " << deltaX << "," << deltaY << ")" << std::endl;
}

void Player::SetOrientation(int orientation) {
    m_orientation = orientation;
    switch (orientation) {
        case 1: m_targetRotation = 0; break;    // North
        case 2: m_targetRotation = 90; break;   // East
        case 3: m_targetRotation = 180; break;  // South
        case 4: m_targetRotation = 270; break;  // West
    }
}

void Player::Look() {
    // Fait tourner le joueur sur lui-même
    m_targetRotation += 360.0f;
    std::cout << "👀 Player " << m_id << " looking around (rotation: " << m_targetRotation << ")" << std::endl;
}

void Player::SpawnAnimation() {
    // Animation de spawn pour les nouveaux crabes créés par fork
    if (m_res) {
        std::cout << "🥚 Player " << m_id << " (team " << m_teamId << ") playing spawn animation" << std::endl;
        int idx = m_res->GetPlayerAnimIndexByName("Crab_Spawn", m_teamId);
        if (idx >= 0 && idx < m_res->GetPlayerAnimCount(m_teamId)) {
            m_currentAnim = idx;
            m_animFrame = 0.0f;
            std::cout << "  ✅ Spawn animation set to index " << idx << " (using model for team " << m_teamId << ")" << std::endl;
        } else {
            std::cout << "  ❌ Crab_Spawn animation not found, using Idle1" << std::endl;
            // Fallback vers Idle1
            idx = m_res->GetPlayerAnimIndexByName("Idle1", m_teamId);
            if (idx >= 0 && idx < m_res->GetPlayerAnimCount(m_teamId)) {
                m_currentAnim = idx;
                m_animFrame = 0.0f;
            }
        }
    }
}

void Player::StartCommand(const std::string& command) {
    m_currentCommand = command;
    m_isExecutingCommand = true;
    m_commandTimer = 0.0f;
    
    // Temps des commandes selon le protocole Zappy (en ticks)
    if (command == "Forward" || command == "Right" || command == "Left" || 
        command == "Look" || command == "Broadcast" || command == "Eject" ||
        command == "Take" || command == "Set") {
        m_commandDuration = 7.0f / m_serverTickRate;  // 7 ticks
    } else if (command == "Inventory") {
        m_commandDuration = 1.0f / m_serverTickRate;  // 1 tick
    } else if (command == "Fork") {
        m_commandDuration = 42.0f / m_serverTickRate; // 42 ticks
    } else if (command == "Incantation") {
        m_commandDuration = 300.0f / m_serverTickRate; // 300 ticks
    } else {
        m_commandDuration = 1.0f / m_serverTickRate;  // Défaut 1 tick
    }
    
    std::cout << "🎮 Player " << m_id << " started command: " << command 
              << " (duration: " << m_commandDuration << "s)" << std::endl;
    
    // Changer l'animation selon la commande
    if (command == "Forward") {
        // Utiliser l'animation spécifique du crabe pour Forward
        if (m_res) {
            std::cout << "🏃 Player " << m_id << " starting run animation" << std::endl;
            int idx = m_res->GetPlayerAnimIndexByName("Run", m_teamId);
            if (idx >= 0 && idx < m_res->GetPlayerAnimCount(m_teamId)) {
                m_currentAnim = idx;
                m_animFrame = 0.0f;
                std::cout << "  ✅ Run animation set to index " << idx << std::endl;
            } else {
                std::cout << "  ❌ Run animation not found, using default" << std::endl;
                SetAnimState(AnimState::RUN);
            }
        }
    } else if (command == "Right") {
        // Rotation de 90° vers la droite
        m_targetRotation += 90.0f;
        SetAnimState(AnimState::IDLE);  // IDLE pour les rotations
    } else if (command == "Left") {
        // Rotation de 90° vers la gauche
        m_targetRotation -= 90.0f;
        SetAnimState(AnimState::IDLE);  // IDLE pour les rotations
    } else if (command == "Look") {
        // Utiliser l'animation spécifique du crabe pour Look
        if (m_res) {
            std::cout << "👀 Player " << m_id << " starting look animation" << std::endl;
            int idx = m_res->GetPlayerAnimIndexByName("SRU_Crab_Flee", m_teamId);
            if (idx >= 0 && idx < m_res->GetPlayerAnimCount(m_teamId)) {
                m_currentAnim = idx;
                m_animFrame = 0.0f;
                std::cout << "  ✅ Look animation set to index " << idx << std::endl;
            } else {
                std::cout << "  ❌ SRU_Crab_Flee animation not found, using default" << std::endl;
                SetAnimState(AnimState::INTERACT);
            }
        }
    } else if (command == "Incantation") {
        // Utiliser l'animation spécifique du crabe pour l'incantation
        if (m_res) {
            std::cout << "🔮 Player " << m_id << " starting incantation animation" << std::endl;
            int idx = m_res->GetPlayerAnimIndexByName("Crab_Stun", m_teamId);
            if (idx >= 0 && idx < m_res->GetPlayerAnimCount(m_teamId)) {
                m_currentAnim = idx;
                m_animFrame = 0.0f;
                std::cout << "  ✅ Incantation animation set to index " << idx << std::endl;
            } else {
                std::cout << "  ❌ Crab_Stun animation not found, using default" << std::endl;
                SetAnimState(AnimState::INCANT);
            }
        }
    } else if (command == "Take" || command == "Set") {
        // Utiliser l'animation spécifique du crabe pour Take/Set
        if (m_res) {
            std::cout << "💃 Player " << m_id << " starting dance animation for " << command << std::endl;
            int idx = m_res->GetPlayerAnimIndexByName("Crab_Dance", m_teamId);
            if (idx >= 0 && idx < m_res->GetPlayerAnimCount(m_teamId)) {
                m_currentAnim = idx;
                m_animFrame = 0.0f;
                std::cout << "  ✅ Dance animation set to index " << idx << std::endl;
            } else {
                std::cout << "  ❌ Crab_Dance animation not found, using default" << std::endl;
                SetAnimState(AnimState::INTERACT);
            }
        }
    } else if (command == "Fork") {
        // Animation de spawn pour les nouveaux crabes
        if (m_res) {
            std::cout << "🥚 Player " << m_id << " spawning new crab" << std::endl;
            int idx = m_res->GetPlayerAnimIndexByName("Crab_Spawn", m_teamId);
            if (idx >= 0 && idx < m_res->GetPlayerAnimCount(m_teamId)) {
                m_currentAnim = idx;
                m_animFrame = 0.0f;
                std::cout << "  ✅ Spawn animation set to index " << idx << std::endl;
            } else {
                std::cout << "  ❌ Crab_Spawn animation not found, using default" << std::endl;
                SetAnimState(AnimState::IDLE);
            }
        }
    } else {
        // Utiliser Idle1 par défaut pour toutes les autres commandes
        if (m_res) {
            int idx = m_res->GetPlayerAnimIndexByName("Idle1", m_teamId);
            if (idx >= 0 && idx < m_res->GetPlayerAnimCount(m_teamId)) {
                m_currentAnim = idx;
                m_animFrame = 0.0f;
                std::cout << "  ✅ Idle1 animation set to index " << idx << std::endl;
            } else {
                std::cout << "  ❌ Idle1 animation not found, using default" << std::endl;
                SetAnimState(AnimState::IDLE);
            }
        }
    }
}

void Player::SetLevel(int level) {
    m_level = level;
}

void Player::SetInventory(const std::vector<int>& inventory) {
    std::cout << "📦 Player " << m_id << " inventory updated:" << std::endl;
    for (int i = 0; i < inventory.size() && i < 7; ++i) {
        std::cout << "  [" << i << "] = " << inventory[i] << std::endl;
    }
    m_inventory = inventory;
}

void Player::SetAnimState(AnimState state, float specialDuration) {
    if (m_animState != state && m_res) {
        std::cout << "🎬 Player " << m_id << " changing animation state to: " << (int)state << std::endl;
        m_animState = state;
        std::string animName;
        switch (state) {
            case AnimState::RUN: animName = "Run"; break;
            case AnimState::CAST: animName = "Cast_Animation"; break;
            case AnimState::INCANT: animName = "Into_Cast"; break;
            case AnimState::CELEBRATE: animName = "Celebrate"; break;
            case AnimState::INTERACT: animName = "Interact"; break;
            default: animName = "Idle1"; break;  // Utiliser Idle1 par défaut
        }
        std::cout << "  Looking for animation: '" << animName << "'" << std::endl;
        int idx = m_res->GetPlayerAnimIndexByName(animName, m_teamId);
        if (idx >= 0 && idx < m_res->GetPlayerAnimCount(m_teamId)) {
            m_currentAnim = idx;
            m_animFrame = 0.0f;
            std::cout << "  ✅ Animation set to index " << idx << ", reset frame to 0" << std::endl;
        } else {
            std::cout << "  ❌ Animation '" << animName << "' not found, using index 0" << std::endl;
            m_currentAnim = 0;  // Fallback vers l'animation 0
            m_animFrame = 0.0f;
        }
        if (state == AnimState::CAST || state == AnimState::INTERACT || state == AnimState::INCANT) {
            m_specialAnimTimer = specialDuration; // durée en secondes
        }
    }
}

void Player::Update(float deltaTime) {
    // Gestion des commandes en cours
    if (m_isExecutingCommand) {
        m_commandTimer += deltaTime;
        if (m_commandTimer >= m_commandDuration) {
            // Commande terminée
            std::cout << "✅ Player " << m_id << " finished command: " << m_currentCommand << std::endl;
            m_isExecutingCommand = false;
            m_currentCommand = "";
            m_commandTimer = 0.0f;
            
            // Retour à l'animation par défaut
            if (m_isMoving) {
                SetAnimState(AnimState::RUN);  // Seulement si il bouge physiquement
            } else {
                SetAnimState(AnimState::IDLE);  // Sinon IDLE
            }
        }
    }
    
    // Mouvement fluide vers la cible
    Vector3 diff = Vector3Subtract(m_targetPosition, m_worldPosition);
    float dist = Vector3Length(diff);
    if (dist > 0.01f) {
        m_worldPosition = Vector3Add(m_worldPosition, Vector3Scale(diff, std::min(1.0f, deltaTime * m_moveSpeed / dist)));
        if (!m_isMoving) {
            std::cout << "🏃 Player " << m_id << " started moving (dist=" << dist << ")" << std::endl;
        }
        m_isMoving = true;
    } else {
        m_worldPosition = m_targetPosition;
        if (m_isMoving) {
            std::cout << "🛑 Player " << m_id << " stopped moving" << std::endl;
        }
        m_isMoving = false;
    }
    // Smooth rotation
    float rotDiff = m_targetRotation - m_rotation;
    if (rotDiff > 180) rotDiff -= 360;
    if (rotDiff < -180) rotDiff += 360;
    m_rotation += rotDiff * 5.0f * deltaTime;
    // Gestion animation spéciale
    if (m_specialAnimTimer > 0.0f) {
        m_specialAnimTimer -= deltaTime;
        if (m_specialAnimTimer <= 0.0f) {
            // Retour à Idle ou Run selon le mouvement
            if (m_isMoving)
                SetAnimState(AnimState::RUN);
            else
                SetAnimState(AnimState::IDLE);
        }
    } else if (!m_isExecutingCommand) {
        // Animation auto Idle/Run seulement si pas de commande en cours
        if (m_isMoving)
            SetAnimState(AnimState::RUN);
        else
            SetAnimState(AnimState::IDLE);
    }
    // Avance la frame d'animation
    if (m_res && m_res->GetPlayerAnimCount(m_teamId) > 0 && m_currentAnim >= 0 && m_currentAnim < m_res->GetPlayerAnimCount(m_teamId)) {
        ModelAnimation* anims = m_res->GetPlayerAnimations(m_teamId);
        if (anims && m_currentAnim < m_res->GetPlayerAnimCount(m_teamId)) {
            float oldFrame = m_animFrame;
            // Animation timing constante à 24 FPS indépendamment de la fréquence serveur
            m_animFrame += deltaTime * 24.0f; // 24 FPS animation stable
            if (m_animFrame >= anims[m_currentAnim].frameCount) {
                m_animFrame = 0.0f;
                std::cout << "🔄 Player " << m_id << " animation loop completed (frame " << oldFrame << " -> 0)" << std::endl;
            }
            // Reduced debug output - only print animation loops and state changes
        }
    }
}

void Player::Draw(Model& model, Color teamColor, bool showBoundingBox) {
    // Animation system with safety checks
    if (m_res && m_res->GetPlayerAnimCount(m_teamId) > 0 &&
        m_currentAnim >= 0 && m_currentAnim < m_res->GetPlayerAnimCount(m_teamId)) {
        ModelAnimation* anims = m_res->GetPlayerAnimations(m_teamId);
        if (anims && m_animFrame >= 0 && m_animFrame < anims[m_currentAnim].frameCount) {
            try {
                UpdateModelAnimation(model, anims[m_currentAnim], (int)m_animFrame);
            } catch (...) {
                std::cout << "❌ Animation update failed for player " << m_id << std::endl;
            }
        }
    }
    // Appliquer une rotation fluide et un scale adapté
    float scale = 0.004f; // Scale original correct
    Vector3 scaling = {scale, scale, scale};
    
    // Debug: log position pour vérifier le rendu
    static int debugCounter = 0;
    if (debugCounter++ % 120 == 0) {  // Toutes les 2 secondes
        std::cout << "🎯 Player " << m_id << " draw at position (" << m_worldPosition.x 
                  << "," << m_worldPosition.y << "," << m_worldPosition.z 
                  << ") scale=" << scale << std::endl;
    }
    // Rendu simplifié sans transformation complexe
    DrawModelEx(model, m_worldPosition, {0,1,0}, m_rotation, scaling, WHITE);
    
    // Debug: dessiner le bounding box si demandé
    if (showBoundingBox) {
        BoundingBox box = GetBoundingBox();
        DrawBoundingBox(box, RED);
    }
}

BoundingBox Player::GetBoundingBox() const {
    // Bounding box adapté au scale 0.004 mais plus grand pour la sélection
    float halfWidth = 0.3f;  // Encore plus grand pour faciliter la sélection
    float height = 0.5f;     // Encore plus grand pour faciliter la sélection
    return {
        { m_worldPosition.x - halfWidth, m_worldPosition.y, m_worldPosition.z - halfWidth },
        { m_worldPosition.x + halfWidth, m_worldPosition.y + height, m_worldPosition.z + halfWidth }
    };
}
