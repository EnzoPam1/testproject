#include "UI.hpp"
#include "World.hpp"
#include "Player.hpp"
#include <algorithm>

UI::UI(int screenWidth, int screenHeight) 
    : m_screenWidth(screenWidth), m_screenHeight(screenHeight) {
}

void UI::Update(float deltaTime) {
    // Update UI animations
}

void UI::Draw(World* world, int selectedPlayerId) {
    // Draw player info if selected
    if (selectedPlayerId >= 0) {
        Player* player = world->GetPlayer(selectedPlayerId);
        if (player) {
            DrawPlayerInfo(player);
        }
    }
    
}

void UI::DrawPlayerInfo(Player* player) {
    DrawPanel(10, m_screenHeight - 250, 600, 240, "Player Info");
    
    int startX = 20;
    int startY = m_screenHeight - 220;
    
    DrawText(TextFormat("ID: #%d", player->GetId()), startX, startY, 20, WHITE);
    DrawText(TextFormat("Team: %d", player->GetTeamId()), 
             startX + 150, startY, 20, WHITE);
    DrawText(TextFormat("Pos: (%d, %d)", player->GetX(), player->GetY()), 
             startX + 300, startY, 20, WHITE);
    
    startY += 40;
    DrawLine(startX, startY, startX + 560, startY, GRAY);
    startY += 20;
    
    DrawText("INVENTORY", startX, startY, 18, YELLOW);
    startY += 30;
    
    const char* res_short[7] = {"FOOD", "LIME", "DERA", "SIBU", 
                               "MEND", "PHIR", "THYS"};
    
    int colWidth = 70;
    int levelX = startX;
    int resourcesX = startX + 100;
    
    DrawText("LEVEL", levelX, startY, 16, GOLD);
    
    for (int i = 0; i < 7; ++i) {
        int colX = resourcesX + i * colWidth;
        DrawText(res_short[i], colX, startY, 14, GOLD);
    }
    
    startY += 25;
    DrawLine(startX, startY, startX + 560, startY, GRAY);
    startY += 10;
    
    const std::vector<int>& inv = player->GetInventory();
    
    Color levelColor = WHITE;
    int level = player->GetLevel();
    switch (level) {
        case 1: levelColor = GRAY; break;
        case 2: levelColor = GREEN; break;
        case 3: levelColor = BLUE; break;
        case 4: levelColor = PURPLE; break;
        case 5: levelColor = ORANGE; break;
        case 6: levelColor = RED; break;
        case 7: levelColor = GOLD; break;
        case 8: levelColor = {255, 215, 0, 255}; break;
        default: levelColor = WHITE; break;
    }
    DrawText(TextFormat("%d", level), levelX, startY, 24, levelColor);
    
    for (int i = 0; i < 7; ++i) {
        int colX = resourcesX + i * colWidth;
        Color textColor = (inv[i] > 0) ? GREEN : LIGHTGRAY;
        Color bgColor = (inv[i] > 0) ? Fade(GREEN, 0.2f) : 
                                      Fade(DARKGRAY, 0.3f);
        
        DrawRectangle(colX - 5, startY - 5, colWidth - 10, 30, bgColor);
        DrawRectangleLines(colX - 5, startY - 5, colWidth - 10, 30, GRAY);
        
        DrawText(TextFormat("%d", inv[i]), colX + colWidth/2 - 10, 
                startY + 5, 18, textColor);
    }
}


void UI::DrawPanel(int x, int y, int width, int height, const std::string& title) {
    DrawRectangle(x, y, width, height, Fade(BLACK, 0.8f));
    DrawRectangleLines(x, y, width, height, WHITE);
    DrawText(title.c_str(), x + 5, y + 5, 16, WHITE);
} 