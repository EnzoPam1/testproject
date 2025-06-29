#pragma once

#include "raylib.h"
#include <string>

class LoginScreen {
public:
    LoginScreen(int screenWidth, int screenHeight);
    ~LoginScreen();

    void Update(float deltaTime);
    void Draw();

    bool IsLoginRequested() const { return m_loginRequested; }
    bool ShouldExit() const { return m_shouldExit; }

    std::string GetHost() const { return m_host; }
    int GetPort() const { return m_port_int; }

    void ResetLoginRequest() { m_loginRequested = false; }

private:
    int m_screenWidth;
    int m_screenHeight;

    std::string m_host;
    std::string m_port;
    int m_port_int;

    bool m_hostInputActive;
    bool m_portInputActive;
    bool m_loginRequested;
    bool m_shouldExit;

    Font m_font;
    float m_blinkTimer;
    bool m_showCursor;

    Rectangle m_hostBox;
    Rectangle m_portBox;
    Rectangle m_connectButton;
    Rectangle m_exitButton;

    void HandleInput();
    void DrawInputBox(Rectangle box, const std::string& text, const std::string& label, bool isActive, bool showCursor);
    void DrawButton(Rectangle box, const std::string& text, Color bgColor, Color textColor);
    bool IsButtonClicked(Rectangle box);
};
