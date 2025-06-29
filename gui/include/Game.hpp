/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
** Main game class with login screen
*/

#pragma once

#include "raylib.h"
#include "Camera.hpp"
#include "World.hpp"
#include "Network.hpp"
#include "UI.hpp"
#include "ResourceManager.hpp"
#include "LoginScreen.hpp"
#include <memory>
#include <string>
#include <cmath>
#include "rlgl.h"

enum class GameState {
    LOGIN,
    CONNECTING,
    PLAYING,
    ERROR
};

class Game {
public:
    Game();
    ~Game();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    std::unique_ptr<GameCamera> m_camera;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Network> m_network;
    std::unique_ptr<UI> m_ui;
    std::unique_ptr<ResourceManager> m_resources;
    std::unique_ptr<LoginScreen> m_loginScreen;

    int m_screenWidth = 1920;
    int m_screenHeight = 1080;
    bool m_isRunning = false;
    GameState m_gameState = GameState::LOGIN;
    int m_selectedPlayerId = -1;
    float m_deltaTime = 0.0f;
    int m_mapWidth = 0;
    int m_mapHeight = 0;
    bool m_showBoundingBoxes = false;
    int m_serverTimeUnit = 100;
    float m_serverTickRate = 1.0f / (m_serverTimeUnit / 1000.0f);

    // Connection parameters from login
    std::string m_host;
    int m_port;

    // Error handling
    std::string m_errorMessage;
    float m_errorTimer = 0.0f;

    void UpdateLogin(float deltaTime);
    void UpdateConnecting(float deltaTime);
    void UpdatePlaying(float deltaTime);
    void UpdateError(float deltaTime);

    void RenderLogin();
    void RenderConnecting();
    void RenderPlaying();
    void RenderError();

    bool ConnectToServer(const std::string& host, int port);
    void HandleInput();
    void HandleMousePicking();
    void ProcessNetworkMessage(const std::string& message);
    bool IsPlayerCommand(const std::string& message) const;
    void ShowError(const std::string& message);

    Model     _skyModel;
    Texture2D _skyTexture;
    Vector3   _skyScale{ -20.0f, 20.0f, 20.0f };  // inversion X pour faces intérieures
    Vector3   _skyPosition{ 0.0f, 0.0f, 0.0f };
};
