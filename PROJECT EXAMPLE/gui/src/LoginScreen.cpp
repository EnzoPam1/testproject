#include "LoginScreen.hpp"
#include <iostream>
#include <algorithm>

LoginScreen::LoginScreen(int screenWidth, int screenHeight)
    : m_screenWidth(screenWidth), m_screenHeight(screenHeight),
      m_host("127.0.0.1"), m_port("4242"), m_port_int(4242),
      m_hostInputActive(false), m_portInputActive(false),
      m_loginRequested(false), m_shouldExit(false),
      m_blinkTimer(0.0f), m_showCursor(true) {

    m_font = GetFontDefault();

    float centerX = m_screenWidth * 0.5f;
    float centerY = m_screenHeight * 0.5f;

    m_hostBox = { centerX - 200, centerY - 60, 400, 50 };
    m_portBox = { centerX - 200, centerY + 20, 400, 50 };
    m_connectButton = { centerX - 120, centerY + 100, 240, 50 };
    m_exitButton = { centerX - 120, centerY + 170, 240, 50 };

    std::cout << "Login screen ready" << std::endl;
}

LoginScreen::~LoginScreen() {
}

void LoginScreen::Update(float deltaTime) {
    HandleInput();

    m_blinkTimer += deltaTime;
    if (m_blinkTimer >= 0.5f) {
        m_showCursor = !m_showCursor;
        m_blinkTimer = 0.0f;
    }

    try {
        m_port_int = std::stoi(m_port);
        if (m_port_int <= 0 || m_port_int > 65535) {
            m_port_int = 4242;
        }
    } catch (...) {
        m_port_int = 4242;
    }
}

void LoginScreen::HandleInput() {
    Vector2 mousePos = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        m_hostInputActive = CheckCollisionPointRec(mousePos, m_hostBox);
        m_portInputActive = CheckCollisionPointRec(mousePos, m_portBox);

        if (IsButtonClicked(m_connectButton)) {
            if (!m_host.empty() && !m_port.empty()) {
                m_loginRequested = true;
                std::cout << "Connecting to " << m_host << ":" << m_port << std::endl;
            }
        }

        if (IsButtonClicked(m_exitButton)) {
            m_shouldExit = true;
        }
    }

    int key = GetCharPressed();
    while (key > 0) {
        if (m_hostInputActive && m_host.length() < 15) {
            if ((key >= 32) && (key <= 125)) {
                m_host += (char)key;
            }
        } else if (m_portInputActive && m_port.length() < 5) {
            if ((key >= '0') && (key <= '9')) {
                m_port += (char)key;
            }
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (m_hostInputActive && !m_host.empty()) {
            m_host.pop_back();
        } else if (m_portInputActive && !m_port.empty()) {
            m_port.pop_back();
        }
    }

    if (IsKeyPressed(KEY_ENTER)) {
        if (!m_host.empty() && !m_port.empty()) {
            m_loginRequested = true;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        m_shouldExit = true;
    }

    if (IsKeyPressed(KEY_TAB)) {
        if (m_hostInputActive) {
            m_hostInputActive = false;
            m_portInputActive = true;
        } else if (m_portInputActive) {
            m_portInputActive = false;
            m_hostInputActive = true;
        } else {
            m_hostInputActive = true;
        }
    }
}

void LoginScreen::Draw() {
    BeginDrawing();

    // Dégradé moderne style League of Legends
    DrawRectangleGradientV(0, 0, m_screenWidth, m_screenHeight,
                          Color{10, 19, 40, 255},    // Bleu foncé en haut
                          Color{25, 38, 56, 255});   // Bleu-gris en bas

    float centerX = m_screenWidth * 0.5f;
    float centerY = m_screenHeight * 0.5f;

    const char* title = "ZAPPY VIEWER";
    int titleSize = 48;
    Vector2 titleMeasure = MeasureTextEx(m_font, title, titleSize, 2);
    float titleX = centerX - titleMeasure.x * 0.5f;
    float titleY = centerY - 180;
    DrawTextEx(m_font, title, Vector2{titleX, titleY}, titleSize, 2, Color{200, 155, 60, 255}); // Or Riot

    const char* subtitle = "Connect to Server";
    int subtitleSize = 24;
    Vector2 subtitleMeasure = MeasureTextEx(m_font, subtitle, subtitleSize, 1);
    float subtitleX = centerX - subtitleMeasure.x * 0.5f;
    float subtitleY = titleY + titleMeasure.y + 20;
    DrawTextEx(m_font, subtitle, Vector2{subtitleX, subtitleY}, subtitleSize, 1, Color{120, 160, 200, 255}); // Bleu clair

    DrawInputBox(m_hostBox, m_host, "Host/IP Address:", m_hostInputActive,
                m_hostInputActive && m_showCursor);
    DrawInputBox(m_portBox, m_port, "Port:", m_portInputActive,
                m_portInputActive && m_showCursor);

    Color connectColor = (!m_host.empty() && !m_port.empty()) ? Color{15, 180, 130, 255} : Color{80, 80, 80, 255}; // Vert gaming
    DrawButton(m_connectButton, "CONNECT", connectColor, WHITE);
    DrawButton(m_exitButton, "EXIT", Color{220, 80, 80, 255}, WHITE); // Rouge gaming

    // Version discrète dans le coin
    const char* version = "v1.0.0";
    Vector2 versionMeasure = MeasureTextEx(m_font, version, 14, 1);
    DrawTextEx(m_font, version,
              Vector2{(float)m_screenWidth - versionMeasure.x - 20.0f, (float)m_screenHeight - 30.0f},
              14, 1, Color{80, 80, 80, 255});

    EndDrawing();
}

void LoginScreen::DrawInputBox(Rectangle box, const std::string& text,
                              const std::string& label, bool isActive, bool showCursor) {
    Vector2 labelMeasure = MeasureTextEx(m_font, label.c_str(), 18, 1);
    float labelX = box.x + (box.width - labelMeasure.x) * 0.5f;
    DrawTextEx(m_font, label.c_str(), Vector2{labelX, box.y - 35}, 18, 1, Color{200, 200, 200, 255}); // Gris clair

    Color boxColor = isActive ? Color{45, 60, 85, 255} : Color{30, 40, 55, 255}; // Bleu foncé gaming
    Color borderColor = isActive ? Color{15, 180, 130, 255} : Color{100, 120, 140, 255}; // Vert/gris gaming

    DrawRectangleRec(box, boxColor);
    DrawRectangleLinesEx(box, isActive ? 3 : 2, borderColor);

    float textX = box.x + 15;
    float textY = box.y + (box.height - 20) * 0.5f;
    DrawTextEx(m_font, text.c_str(), Vector2{textX, textY}, 20, 1, Color{220, 220, 220, 255}); // Blanc cassé

    if (showCursor) {
        Vector2 textMeasure = MeasureTextEx(m_font, text.c_str(), 20, 1);
        float cursorX = textX + textMeasure.x + 2;
        DrawLineEx(Vector2{cursorX, textY + 2}, Vector2{cursorX, textY + 18}, 2, Color{15, 180, 130, 255}); // Vert gaming
    }
}

void LoginScreen::DrawButton(Rectangle box, const std::string& text, Color bgColor, Color textColor) {
    Vector2 mousePos = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mousePos, box);

    if (isHovered && bgColor.r > 50) {
        bgColor.r = std::min(255, bgColor.r + 30);
        bgColor.g = std::min(255, bgColor.g + 30);
        bgColor.b = std::min(255, bgColor.b + 30);
    }

    // Ombre plus prononcée style gaming
    DrawRectangleRec(Rectangle{box.x + 4, box.y + 4, box.width, box.height}, Color{0, 0, 0, 120});
    DrawRectangleRec(box, bgColor);
    DrawRectangleLinesEx(box, 2, Fade(bgColor, 0.6f));

    Vector2 textMeasure = MeasureTextEx(m_font, text.c_str(), 20, 1);
    float textX = box.x + (box.width - textMeasure.x) * 0.5f;
    float textY = box.y + (box.height - textMeasure.y) * 0.5f;
    DrawTextEx(m_font, text.c_str(), Vector2{textX, textY}, 20, 1, textColor);
}

bool LoginScreen::IsButtonClicked(Rectangle box) {
    Vector2 mousePos = GetMousePosition();
    return CheckCollisionPointRec(mousePos, box) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}
