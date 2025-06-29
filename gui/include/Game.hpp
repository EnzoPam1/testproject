/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
** Main game class
*/

#pragma once

#include "raylib.h"
#include "Camera.hpp"
#include "World.hpp"
#include "Network.hpp"
#include "UI.hpp"
#include "ResourceManager.hpp"
#include <memory>
#include <string>

class Game {
public:
    Game();
    ~Game();

    bool Initialize(const std::string& host, int port);
    void Run();
    void Shutdown();

private:
    std::unique_ptr<GameCamera> m_camera;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Network> m_network;
    std::unique_ptr<UI> m_ui;
    std::unique_ptr<ResourceManager> m_resources;

    int m_screenWidth = 1920;
    int m_screenHeight = 1080;
    bool m_isRunning = false;
    int m_selectedPlayerId = -1;
    float m_deltaTime = 0.0f;
    int m_mapWidth = 0;
    int m_mapHeight = 0;
    bool m_showBoundingBoxes = false;
    int m_serverTimeUnit = 100;
    float m_serverTickRate = 1.0f / (m_serverTimeUnit / 1000.0f);

    void HandleInput();
    void Update(float deltaTime);
    void Render();
    void HandleMousePicking();
    void ProcessNetworkMessage(const std::string& message);
    bool IsPlayerCommand(const std::string& message) const;
}; 