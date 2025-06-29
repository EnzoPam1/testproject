#pragma once

#include "raylib.h"
#include <string>

class World;
class Player;

class UI {
public:
    UI(int screenWidth, int screenHeight);
    ~UI() = default;

    void Update(float deltaTime);
    void Draw(World* world, int selectedPlayerId);

private:
    int m_screenWidth;
    int m_screenHeight;
    
    // UI elements
    void DrawPlayerInfo(Player* player);
    void DrawTeamList(World* world);
    void DrawEventLog();
    
    // Helper
    void DrawPanel(int x, int y, int width, int height, const std::string& title);
}; 