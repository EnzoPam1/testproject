/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
** Game implementation
*/

#include "Game.hpp"
#include <iostream>

Game::Game() {
    m_resources = std::make_unique<ResourceManager>();
    m_camera = std::make_unique<GameCamera>(m_screenWidth, m_screenHeight);
    m_world = std::make_unique<World>(m_resources.get());
    m_network = std::make_unique<Network>();
    m_ui = std::make_unique<UI>(m_screenWidth, m_screenHeight);
}

Game::~Game() = default;

bool Game::Initialize(const std::string& host, int port) {
    InitWindow(m_screenWidth, m_screenHeight, "Zappy - 3D Viewer");
    SetTargetFPS(60);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    
    if (!m_resources->LoadAll()) {
        std::cerr << "Failed to load resources\n";
        return false;
    }
    
    if (!m_network->Connect(host, port)) {
        std::cerr << "Failed to connect to server\n";
        return false;
    }
    
    m_network->SendCommand("GRAPHIC");
    m_network->SendCommand("msz");
    m_network->SendCommand("sgt");
    
    std::string response;
    int timeout = 0;
    const int maxTimeout = 100;
    
    while (timeout < maxTimeout && m_network->ReceiveMessage(response)) {
        if (response.substr(0, 3) == "msz") {
            int width, height;
            if (sscanf(response.c_str(), "msz %d %d", &width, &height) == 2) {
                m_mapWidth = width;
                m_mapHeight = height;
                float tileSize = std::min(m_screenWidth / (float)width, 
                                        m_screenHeight / (float)height) * 1.0f;
                m_world->Initialize(width, height, tileSize);
                m_camera->SetBounds(0, width * tileSize, 0, height * tileSize);
                m_camera->SetTarget({width * tileSize / 2.0f, 0, 
                                   height * tileSize / 2.0f});
                break;
            }
        }
        timeout++;
    }
    
    if (m_mapWidth == 0 || m_mapHeight == 0) {
        m_mapWidth = 10;
        m_mapHeight = 10;
        float tileSize = 2.0f;
        m_world->Initialize(m_mapWidth, m_mapHeight, tileSize);
        m_camera->SetBounds(0, m_mapWidth * tileSize, 0, m_mapHeight * tileSize);
        m_camera->SetTarget({m_mapWidth * tileSize / 2.0f, 0, 
                           m_mapHeight * tileSize / 2.0f});
    }
    
    m_isRunning = true;
    return true;
}

void Game::Run() {
    while (m_isRunning && !WindowShouldClose()) {
        m_deltaTime = GetFrameTime();
        HandleInput();
        Update(m_deltaTime);
        Render();
    }
}

void Game::Shutdown() {
    EnableCursor();
    m_network->Disconnect();
    CloseWindow();
}

void Game::HandleInput() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        m_isRunning = false;
    }
    
    if (IsKeyPressed(KEY_B)) {
        m_showBoundingBoxes = !m_showBoundingBoxes;
    }
    
    bool ctrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || 
                      IsKeyDown(KEY_RIGHT_CONTROL);
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !ctrlPressed) {
        HandleMousePicking();
    }
}

void Game::Update(float deltaTime) {
    m_camera->Update(deltaTime);
    
    std::string message;
    while (m_network->ReceiveMessage(message)) {
        ProcessNetworkMessage(message);
    }
    
    m_world->Update(m_deltaTime);
    m_ui->Update(m_deltaTime);
}

void Game::ProcessNetworkMessage(const std::string& message) {
    if (message.substr(0, 3) == "ppo") {
        int id, x, y, o;
        if (sscanf(message.c_str(), "ppo #%d %d %d %d", &id, &x, &y, &o) == 4) {
            m_world->UpdatePlayer(id, x, y, o);
        }
    } else if (message.substr(0, 3) == "pnw") {
        int id, x, y, o, l;
        char team[256];
        if (sscanf(message.c_str(), "pnw #%d %d %d %d %d %s", 
                   &id, &x, &y, &o, &l, team) == 6) {
            std::string teamName(team);
            int teamId = (teamName.find("2") != std::string::npos) ? 1 : 0;
            m_world->AddPlayer(id, teamId, x, y, o, l);
        }
    } else if (message.substr(0, 3) == "bct") {
        int x, y, food, stones[6];
        if (sscanf(message.c_str(), "bct %d %d %d %d %d %d %d %d %d",
                   &x, &y, &food, &stones[0], &stones[1], &stones[2],
                   &stones[3], &stones[4], &stones[5]) == 9) {
            TileData data;
            data.food = food;
            for (int i = 0; i < 6; i++) {
                data.stones[i] = stones[i];
            }
            m_world->UpdateTile(x, y, data);
        }
    } else if (message.substr(0, 3) == "pin") {
        int id, x, y, inv[7];
        if (sscanf(message.c_str(), "pin #%d %d %d %d %d %d %d %d %d %d",
                   &id, &x, &y, &inv[0], &inv[1], &inv[2], &inv[3], 
                   &inv[4], &inv[5], &inv[6]) == 10) {
            std::vector<int> inventory(inv, inv + 7);
            m_world->UpdatePlayerInventory(id, inventory);
        }
    } else if (message.substr(0, 4) == "look") {
        int id;
        if (sscanf(message.c_str(), "look #%d", &id) == 1) {
            m_world->PlayerLook(id);
        }
    } else if (message.substr(0, 3) == "sgt") {
        int timeUnit;
        if (sscanf(message.c_str(), "sgt %d", &timeUnit) == 1) {
            m_serverTimeUnit = timeUnit;
            m_serverTickRate = 1.0f / (timeUnit / 1000.0f);
            m_world->SetServerTickRate(m_serverTickRate);
        }
    } else if (IsPlayerCommand(message)) {
        int id;
        char command[256];
        if (sscanf(message.c_str(), "%s #%d", command, &id) == 2) {
            m_world->StartPlayerCommand(id, std::string(command));
        }
    }
}

bool Game::IsPlayerCommand(const std::string& message) const {
    return message.substr(0, 7) == "Forward" || 
           message.substr(0, 5) == "Right" || 
           message.substr(0, 4) == "Left" || 
           message.substr(0, 4) == "Look" ||
           message.substr(0, 8) == "Inventory" || 
           message.substr(0, 9) == "Broadcast" ||
           message.substr(0, 4) == "Fork" || 
           message.substr(0, 5) == "Eject" ||
           message.substr(0, 3) == "Take" || 
           message.substr(0, 3) == "Set" ||
           message.substr(0, 11) == "Incantation";
}

void Game::Render() {
    BeginDrawing();
    ClearBackground(BLACK);
    
    m_camera->BeginMode3D();
    {
        m_world->Draw(m_showBoundingBoxes, m_camera->GetCamera());
    }
    m_camera->EndMode3D();
    
    m_ui->Draw(m_world.get(), m_selectedPlayerId);
    DrawFPS(10, 10);
    
    EndDrawing();
}

void Game::HandleMousePicking() {
    Ray ray = m_camera->GetMouseRay();
    int playerId = m_world->GetPlayerAt(ray);
    
    if (playerId >= 0) {
        m_selectedPlayerId = playerId;
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "pin #%d", playerId);
        m_network->SendCommand(cmd);
    } else {
        m_selectedPlayerId = -1;
    }
}