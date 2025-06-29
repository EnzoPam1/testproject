#include "ResourceManager.hpp"
#include <iostream>
#include "raymath.h"

ResourceManager::ResourceManager() {
    m_playerAnimationsPtr1 = nullptr;
    m_playerAnimationsPtr2 = nullptr;
    m_playerAnimCount1 = 0;
    m_playerAnimCount2 = 0;
}

ResourceManager::~ResourceManager() {
    UnloadAll();
}

bool ResourceManager::LoadAll() {
    return LoadModels() && LoadTextures() && LoadFonts();
}

void ResourceManager::UnloadAll() {
    for (auto& [name, model] : m_models) {
        if (model.meshCount > 0) {
            UnloadModel(model);
        }
    }
    m_models.clear();
    
    for (auto& [name, texture] : m_textures) {
        if (texture.id > 0) {
            UnloadTexture(texture);
        }
    }
    m_textures.clear();
    
    if (m_playerAnimationsPtr1 && m_playerAnimCount1 > 0) {
        UnloadModelAnimations(m_playerAnimationsPtr1, m_playerAnimCount1);
        m_playerAnimationsPtr1 = nullptr;
        m_playerAnimCount1 = 0;
    }
    if (m_playerAnimationsPtr2 && m_playerAnimCount2 > 0) {
        UnloadModelAnimations(m_playerAnimationsPtr2, m_playerAnimCount2);
        m_playerAnimationsPtr2 = nullptr;
        m_playerAnimCount2 = 0;
    }
    
    m_playerAnimNames1.clear();
    m_playerAnimNames2.clear();
    m_animationMap1.clear();
    m_animationMap2.clear();
    
    UnloadFont(m_font);
}

bool ResourceManager::LoadModels() {
    Model playerModel1 = LoadModel("gui/gui/assets/models/scuttle_crab.glb");
    Model playerModel2 = LoadModel("gui/gui/assets/models/scuttle_crab2.glb");

    if (playerModel1.meshCount > 0) {
        m_models["player1"] = playerModel1;
    } else {
        m_models["player1"] = LoadModelFromMesh(GenMeshCube(1.0f, 2.0f, 1.0f));
    }

    if (playerModel2.meshCount > 0) {
        m_models["player2"] = playerModel2;
    } else {
        m_models["player2"] = LoadModelFromMesh(GenMeshCube(1.0f, 2.0f, 1.0f));
    }

    m_models["tile"] = LoadModelFromMesh(GenMeshCube(1.0f, 0.1f, 1.0f));


    std::string resourcePaths[7] = {
        "gui/gui/assets/models/yellow.glb",
        "gui/gui/assets/models/azur.glb",
        "gui/gui/assets/models/diiamond.glb",
        "gui/gui/assets/models/orange.glb",
        "gui/gui/assets/models/green.glb",
        "gui/gui/assets/models/purple.glb",
        "gui/gui/assets/models/red.glb"
    };
    
    for (int i = 0; i < 7; i++) {
        Model resourceModel = LoadModel(resourcePaths[i].c_str());
        
        if (resourceModel.meshCount > 0) {
            resourceModel.transform = MatrixScale(0.25f, 0.25f, 0.25f);
            m_models["resource_" + std::to_string(i)] = resourceModel;
        } else {
            m_models["resource_" + std::to_string(i)] = 
                LoadModelFromMesh(GenMeshSphere(0.15f, 8, 8));
        }
    }

    if (m_models["player1"].meshCount > 0) {
        ModelAnimation* anims1 = LoadModelAnimations(
            "gui/gui/assets/models/scuttle_crab.glb", &m_playerAnimCount1);
        if (anims1 && m_playerAnimCount1 > 0) {
            m_playerAnimationsPtr1 = anims1;
            m_playerAnimNames1.clear();
            m_animationMap1.clear();

            for (int i = 0; i < m_playerAnimCount1; ++i) {
                if (anims1[i].name) {
                    std::string name = std::string(anims1[i].name);
                    m_playerAnimNames1.push_back(name);
                }
            }
        } else {
            m_playerAnimationsPtr1 = nullptr;
            m_playerAnimCount1 = 0;
        }
    }

    if (m_models["player2"].meshCount > 0) {
        ModelAnimation* anims2 = LoadModelAnimations(
            "gui/gui/assets/models/scuttle_crab2.glb", &m_playerAnimCount2);
        if (anims2 && m_playerAnimCount2 > 0) {
            m_playerAnimationsPtr2 = anims2;
            m_playerAnimNames2.clear();
            m_animationMap2.clear();

            for (int i = 0; i < m_playerAnimCount2; ++i) {
                if (anims2[i].name) {
                    std::string name = std::string(anims2[i].name);
                    m_playerAnimNames2.push_back(name);
                }
            }
        } else {
            m_playerAnimationsPtr2 = nullptr;
            m_playerAnimCount2 = 0;
        }
    }

    return true;
}

bool ResourceManager::LoadTextures() {
    Image img = GenImageColor(64, 64, GREEN);
    m_textures["grass"] = LoadTextureFromImage(img);
    UnloadImage(img);
    return true;
}

bool ResourceManager::LoadFonts() {
    m_font = GetFontDefault();
    return true;
}

Model& ResourceManager::GetResourceModel(int type) {
    return m_models["resource_" + std::to_string(type)];
}

Texture2D& ResourceManager::GetTexture(const std::string& name) {
    return m_textures[name];
}

Color ResourceManager::GetTeamColor(int teamId) {
    Color colors[] = { RED, BLUE, GREEN, YELLOW, PURPLE, ORANGE };
    return colors[teamId % 6];
}

Color ResourceManager::GetResourceColor(int type) {
    Color colors[7] = {
        YELLOW, SKYBLUE, GRAY, ORANGE, LIME, PURPLE, RED
    };
    return colors[type % 7];
}

int ResourceManager::GetPlayerAnimIndexByName(const std::string& name, 
                                             int teamId) const {
    const std::vector<std::string>& animNames = 
        (teamId == 1) ? m_playerAnimNames2 : m_playerAnimNames1;
    
    if (animNames.empty()) {
        return 0;
    }
    
    for (int i = 0; i < animNames.size(); ++i) {
        if (animNames[i] == name) {
            return i;
        }
    }
    return 0;
}
