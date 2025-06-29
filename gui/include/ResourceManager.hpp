#pragma once

#include "raylib.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    bool LoadAll();
    void UnloadAll();

    Model& GetPlayerModel(int teamId = 0) { 
        std::string key = (teamId == 0) ? "player1" : "player2";
        auto it = m_models.find(key);
        if (it != m_models.end()) {
            return it->second;
        }
        return m_models["player1"];
    }
    Model& GetTileModel() { return m_models["tile"]; }
    Model& GetResourceModel(int type);
    Texture2D& GetTexture(const std::string& name);
    Color GetTeamColor(int teamId);
    Color GetResourceColor(int type);
    Font& GetFont() { return m_font; }

    ModelAnimation* GetPlayerAnimations(int teamId = 0) { 
        return (teamId == 1) ? m_playerAnimationsPtr2 : 
                              m_playerAnimationsPtr1;
    }
    int GetPlayerAnimCount(int teamId = 0) const { 
        return (teamId == 1) ? m_playerAnimCount2 : m_playerAnimCount1;
    }
    int GetPlayerAnimIndexByName(const std::string& name, 
                                int teamId = 0) const;
    int GetAnimationIndex(const std::string& key) const {
        auto it = m_animationMap.find(key);
        return (it != m_animationMap.end()) ? it->second : 0;
    }

private:
    std::unordered_map<std::string, Model> m_models;
    std::unordered_map<std::string, Texture2D> m_textures;
    std::unordered_map<std::string, int> m_animationMap;
    Font m_font;

    ModelAnimation* m_playerAnimationsPtr1 = nullptr;
    int m_playerAnimCount1 = 0;
    std::vector<std::string> m_playerAnimNames1;

    ModelAnimation* m_playerAnimationsPtr2 = nullptr;
    int m_playerAnimCount2 = 0;
    std::vector<std::string> m_playerAnimNames2;

    std::unordered_map<std::string, int> m_animationMap1;
    std::unordered_map<std::string, int> m_animationMap2;

    bool LoadModels();
    bool LoadTextures();
    bool LoadFonts();
};
