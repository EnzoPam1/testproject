/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
** Game implementation with login screen
*/

#include "Game.hpp"
#include <iostream>

Game::Game() {
    // Don't create objects that use Raylib in constructor
    // They will be created in Initialize() after Raylib is ready
}

Game::~Game() = default;

bool Game::Initialize() {
    // Initialize Raylib FIRST
    InitWindow(m_screenWidth, m_screenHeight, "Zappy - 3D Viewer");
    SetTargetFPS(60);
    // Note: SetConfigFlags should be called BEFORE InitWindow, but it's too late now

    // NOW create objects that depend on Raylib
    m_resources = std::make_unique<ResourceManager>();
    m_camera = std::make_unique<GameCamera>(m_screenWidth, m_screenHeight);
    m_world = std::make_unique<World>(m_resources.get());
    m_network = std::make_unique<Network>();
    m_ui = std::make_unique<UI>(m_screenWidth, m_screenHeight);
    m_loginScreen = std::make_unique<LoginScreen>(m_screenWidth, m_screenHeight);

    // Load resources AFTER everything is created
    if (!m_resources->LoadAll()) {
        std::cerr << "Failed to load resources\n";
        return false;
    }

    _skyTexture = LoadTexture("gui/gui/assets/output.png");
    std::cout << "[DEBUG] skyTexture.id = " << _skyTexture.id << " (" << _skyTexture.width << "x" << _skyTexture.height << ")\n";

    // Crée la géométrie sphérique avec UV
    Mesh sphereMesh = GenMeshSphere(1.0f, 32, 32);
    _skyModel       = LoadModelFromMesh(sphereMesh);
    // Applique la texture sur le matériau principal
    _skyModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = _skyTexture;

    m_isRunning = true;
    m_gameState = GameState::LOGIN;

    std::cout << "🎮 Game initialized in LOGIN state" << std::endl;
    return true;
}

void Game::Run() {
    while (m_isRunning && !WindowShouldClose()) {
        m_deltaTime = GetFrameTime();

        switch (m_gameState) {
            case GameState::LOGIN:
                UpdateLogin(m_deltaTime);
                RenderLogin();
                break;
            case GameState::CONNECTING:
                UpdateConnecting(m_deltaTime);
                RenderConnecting();
                break;
            case GameState::PLAYING:
                UpdatePlaying(m_deltaTime);
                RenderPlaying();
                break;
            case GameState::ERROR:
                UpdateError(m_deltaTime);
                RenderError();
                break;
        }
    }
}

void Game::UpdateLogin(float deltaTime) {
    m_loginScreen->Update(deltaTime);

    if (m_loginScreen->ShouldExit()) {
        m_isRunning = false;
        return;
    }

    if (m_loginScreen->IsLoginRequested()) {
        m_host = m_loginScreen->GetHost();
        m_port = m_loginScreen->GetPort();

        std::cout << "🔗 Attempting connection to " << m_host << ":" << m_port << std::endl;

        m_gameState = GameState::CONNECTING;
        m_loginScreen->ResetLoginRequest();
    }
}

void Game::UpdateConnecting(float deltaTime) {
    static bool connectionStarted = false;

    if (!connectionStarted) {
        if (ConnectToServer(m_host, m_port)) {
            std::cout << "✅ Connection successful, initializing game..." << std::endl;
            m_gameState = GameState::PLAYING;
        } else {
            ShowError("Failed to connect to server.\nPlease check host and port.");
            m_gameState = GameState::ERROR;
        }
        connectionStarted = true;
    }

    // Reset for next connection attempt
    if (m_gameState != GameState::CONNECTING) {
        connectionStarted = false;
    }
}

void Game::UpdatePlaying(float deltaTime) {
    // Handle input (including ESC to go back to login)
    if (IsKeyPressed(KEY_F1)) { // F1 to go back to login
        m_network->Disconnect();
        m_gameState = GameState::LOGIN;
        return;
    }

    HandleInput();

    m_camera->Update(deltaTime);

    std::string message;
    while (m_network->ReceiveMessage(message)) {
        ProcessNetworkMessage(message);
    }

    m_world->Update(m_deltaTime);
    m_ui->Update(m_deltaTime);
}

void Game::UpdateError(float deltaTime) {
    m_errorTimer += deltaTime;

    // Go back to login after 3 seconds or on any key press
    if (m_errorTimer > 3.0f || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
        m_gameState = GameState::LOGIN;
        m_errorTimer = 0.0f;
    }
}

void Game::RenderLogin() {
    m_loginScreen->Draw();
}

void Game::RenderConnecting() {
    BeginDrawing();
    ClearBackground(Color{20, 20, 30, 255});

    // Centered connecting message
    const char* message = "Connecting to server...";
    int fontSize = 32;
    Vector2 textSize = MeasureTextEx(GetFontDefault(), message, fontSize, 2);
    float x = (m_screenWidth - textSize.x) * 0.5f;
    float y = (m_screenHeight - textSize.y) * 0.5f;

    DrawTextEx(GetFontDefault(), message, Vector2{x, y}, fontSize, 2, WHITE);

    // Simple loading animation
    static float rotation = 0.0f;
    rotation += 120.0f * m_deltaTime; // 120 degrees per second

    Vector2 center = {m_screenWidth * 0.5f, y + textSize.y + 50};
    DrawRing(center, 20, 25, 0, rotation, 32, BLUE);

    EndDrawing();
}

void Game::RenderPlaying() {
    BeginDrawing();
    ClearBackground(BLACK);

    m_camera->BeginMode3D();
    {
        Camera3D cam = m_camera->GetCamera();
        rlDisableDepthTest();
        DrawModelEx(
            _skyModel,
            cam.position,
            (Vector3){0,1,0},
            0.0f,
            _skyScale,
            WHITE                           // teinte
        );
        rlEnableDepthTest();
        m_world->Draw(m_showBoundingBoxes, m_camera->GetCamera());
    }
    m_camera->EndMode3D();

    m_ui->Draw(m_world.get(), m_selectedPlayerId);
    DrawFPS(10, 10);

    DrawText("F1: Back to Login | B: Toggle HitBoxes | ESC: Exit",
             10, m_screenHeight - 30, 16, LIGHTGRAY);

    EndDrawing();
}

void Game::RenderError() {
    BeginDrawing();
    ClearBackground(Color{40, 20, 20, 255});

    // Error title
    const char* title = "Connection Error";
    int titleSize = 36;
    Vector2 titleMeasure = MeasureTextEx(GetFontDefault(), title, titleSize, 2);
    float titleX = (m_screenWidth - titleMeasure.x) * 0.5f;
    float titleY = m_screenHeight * 0.4f;

    DrawTextEx(GetFontDefault(), title, Vector2{titleX, titleY}, titleSize, 2, RED);

    // Error message
    int messageSize = 20;
    Vector2 messageMeasure = MeasureTextEx(GetFontDefault(), m_errorMessage.c_str(), messageSize, 1);
    float messageX = (m_screenWidth - messageMeasure.x) * 0.5f;
    float messageY = titleY + titleMeasure.y + 30;

    DrawTextEx(GetFontDefault(), m_errorMessage.c_str(), Vector2{messageX, messageY}, messageSize, 1, WHITE);

    // Instructions
    const char* instructions = "Press SPACE or ENTER to return to login";
    int instrSize = 16;
    Vector2 instrMeasure = MeasureTextEx(GetFontDefault(), instructions, instrSize, 1);
    float instrX = (m_screenWidth - instrMeasure.x) * 0.5f;
    float instrY = messageY + messageMeasure.y + 50;

    DrawTextEx(GetFontDefault(), instructions, Vector2{instrX, instrY}, instrSize, 1, LIGHTGRAY);

    EndDrawing();
}


bool Game::ConnectToServer(const std::string& host, int port) {
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
        if (response.rfind("msz", 0) == 0) {
            int width, height;
            if (sscanf(response.c_str(), "msz %d %d", &width, &height) == 2) {
                m_mapWidth  = width;
                m_mapHeight = height;
                float tileSize = std::min(
                    m_screenWidth  / (float)width,
                    m_screenHeight / (float)height
                );

                // Initialisation de la map
                m_world->Initialize(width, height, tileSize);
                m_camera->SetBounds(0, width * tileSize,
                                    0, height * tileSize);
                m_camera->SetTarget({
                    width * tileSize / 2.0f,
                    0,
                    height * tileSize / 2.0f
                });

                {
                    float mapW  = m_mapWidth  * tileSize;
                    float mapH  = m_mapHeight * tileSize;
                    // Rayon minimal pour englober la diagonale du plateau
                    float radius = 0.5f * std::sqrt(mapW*mapW + mapH*mapH);
                    // Ajouter une marge de 2 tuiles
                    radius *= 3.0f;
                    _skyScale = { -radius, radius, radius };
                }

                break;
            }
        }
        timeout++;
    }

    if (m_mapWidth == 0 || m_mapHeight == 0) {
        m_mapWidth  = 10;
        m_mapHeight = 10;
        float tileSize = 2.0f;
        m_world->Initialize(m_mapWidth, m_mapHeight, tileSize);
        m_camera->SetBounds(0, m_mapWidth * tileSize,
                            0, m_mapHeight * tileSize);
        m_camera->SetTarget({
            m_mapWidth  * tileSize / 2.0f,
            0,
            m_mapHeight * tileSize / 2.0f
        });

        {
            float mapW  = m_mapWidth  * tileSize;
            float mapH  = m_mapHeight * tileSize;
            float radius = 0.5f * std::sqrt(mapW*mapW + mapH*mapH);
            radius += 2 * tileSize;
            _skyScale = { -radius, radius, radius };
        }
    }

    return true;
}

void Game::HandleInput() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (m_network) {
            m_network->Disconnect();
        }
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

void Game::ShowError(const std::string& message) {
    m_errorMessage = message;
    m_errorTimer = 0.0f;
}

void Game::Shutdown() {
    EnableCursor();
    if (m_network) {
        m_network->Disconnect();
    }
    UnloadModel(_skyModel);
    UnloadTexture(_skyTexture);
    CloseWindow();
}
