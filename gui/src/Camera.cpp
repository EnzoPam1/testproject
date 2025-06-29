/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
** Auto-chess style camera implementation
*/

#include "Camera.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

GameCamera::GameCamera(int screenWidth, int screenHeight) {
    // Initialize camera for bigger world scale
    m_camera.position = { 40.0f, 40.0f, 40.0f };  // Start further back for bigger world
    m_camera.target = { 0.0f, 0.0f, 0.0f };       // Look at world center
    m_camera.up = { 0.0f, 1.0f, 0.0f };           // Up vector
    m_camera.fovy = 45.0f;                         // Field of view
    m_camera.projection = CAMERA_PERSPECTIVE;     // Projection type
    
    m_targetPosition = m_camera.target;
    m_lastMousePos = GetMousePosition();
    
    std::cout << "🎥 Camera initialized. Controls:" << std::endl;
    std::cout << "  - Ctrl + Left Click: Rotate camera" << std::endl;
    std::cout << "  - ZQSD: Move camera (French layout)" << std::endl; 
    std::cout << "  - Mouse wheel: Zoom" << std::endl;
    std::cout << "  - Left Click: Select player" << std::endl;
    std::cout << "  - R: Reset camera to center" << std::endl;
}

void GameCamera::Update(float deltaTime) {
    HandleMouseInput();
    HandleKeyboardInput(deltaTime);
    
    // Smooth camera movement
    m_camera.target.x += (m_targetPosition.x - m_camera.target.x) * m_smoothSpeed * deltaTime;
    m_camera.target.z += (m_targetPosition.z - m_camera.target.z) * m_smoothSpeed * deltaTime;
    
    UpdateCameraPosition();
}

void GameCamera::BeginMode3D() {
    ::BeginMode3D(m_camera);
}

void GameCamera::EndMode3D() {
    ::EndMode3D();
}

void GameCamera::SetTarget(Vector3 target) {
    m_targetPosition = target;
}

void GameCamera::SetBounds(float minX, float maxX, float minZ, float maxZ) {
    m_minX = minX;
    m_maxX = maxX;
    m_minZ = minZ;
    m_maxZ = maxZ;
}

Ray GameCamera::GetMouseRay() {
    return ::GetMouseRay(GetMousePosition(), m_camera);
}

void GameCamera::HandleMouseInput() {
    Vector2 mousePos = GetMousePosition();
    Vector2 mouseDelta = {
        mousePos.x - m_lastMousePos.x,
        mousePos.y - m_lastMousePos.y
    };
    
    // Zoom with mouse wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        m_distance -= wheel * 5.0f;  // Zoom plus rapide
        m_distance = std::clamp(m_distance, m_minDistance, m_maxDistance);
        std::cout << "🔍 Zoom: distance = " << m_distance << " (wheel = " << wheel << ")" << std::endl;
    }
    
    // Rotate camera with Ctrl + Left mouse button ONLY
    bool ctrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shouldRotate = ctrlPressed && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    
    if (shouldRotate) {
        if (!m_isRotating) {
            m_isRotating = true;
            DisableCursor();
        }
        
        m_rotation -= mouseDelta.x * 0.5f;
        m_angle -= mouseDelta.y * 0.5f;
        m_angle = std::clamp(m_angle, 10.0f, 80.0f);
    } else if (m_isRotating) {
        m_isRotating = false;
        EnableCursor();
    }
    
    // Pan camera with middle mouse button
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        if (!m_isPanning) {
            m_isPanning = true;
        }
        
        // Calculate pan based on camera orientation
        float panSpeed = m_distance * 0.01f;
        float radRotation = m_rotation * DEG2RAD;
        
        m_targetPosition.x -= (mouseDelta.x * cosf(radRotation) + mouseDelta.y * sinf(radRotation)) * panSpeed;
        m_targetPosition.z -= (-mouseDelta.x * sinf(radRotation) + mouseDelta.y * cosf(radRotation)) * panSpeed;
    } else {
        m_isPanning = false;
    }
    
    m_lastMousePos = mousePos;
}

void GameCamera::HandleKeyboardInput(float deltaTime) {
    float moveSpeed = 20.0f * deltaTime;
    
    // Calculate movement direction based on camera rotation
    float radRotation = m_rotation * DEG2RAD;
    Vector3 forward = { sinf(radRotation), 0, cosf(radRotation) };
    Vector3 right = { cosf(radRotation), 0, -sinf(radRotation) };
    
    // ZQSD movement (French AZERTY layout) with debug
    if (IsKeyDown(KEY_Z)) {  // Forward (Z = W in AZERTY)
        m_targetPosition.x -= forward.x * moveSpeed;
        m_targetPosition.z -= forward.z * moveSpeed;
        static int debugCounter = 0;
        if (debugCounter++ % 30 == 0) std::cout << "⬆️ Z pressed - moving forward" << std::endl;
    }
    if (IsKeyDown(KEY_S)) {  // Backward
        m_targetPosition.x += forward.x * moveSpeed;
        m_targetPosition.z += forward.z * moveSpeed;
        static int debugCounter = 0;
        if (debugCounter++ % 30 == 0) std::cout << "⬇️ S pressed - moving backward" << std::endl;
    }
    if (IsKeyDown(KEY_Q)) {  // Left (Q = A in AZERTY)
        m_targetPosition.x -= right.x * moveSpeed;
        m_targetPosition.z -= right.z * moveSpeed;
        static int debugCounter = 0;
        if (debugCounter++ % 30 == 0) std::cout << "⬅️ Q pressed - moving left" << std::endl;
    }
    if (IsKeyDown(KEY_D)) {  // Right
        m_targetPosition.x += right.x * moveSpeed;
        m_targetPosition.z += right.z * moveSpeed;
        static int debugCounter = 0;
        if (debugCounter++ % 30 == 0) std::cout << "➡️ D pressed - moving right" << std::endl;
    }
    
    // R to reset camera to center
    if (IsKeyPressed(KEY_R)) {
        m_targetPosition.x = 0.0f;
        m_targetPosition.z = 0.0f;
        std::cout << "🎯 Camera reset to center" << std::endl;
    }
    
    // Apply bounds
    m_targetPosition.x = std::clamp(m_targetPosition.x, m_minX, m_maxX);
    m_targetPosition.z = std::clamp(m_targetPosition.z, m_minZ, m_maxZ);
}

void GameCamera::UpdateCameraPosition() {
    // Calculate camera position based on target, distance, angle and rotation
    float radAngle = m_angle * DEG2RAD;
    float radRotation = m_rotation * DEG2RAD;
    
    m_camera.position.x = m_camera.target.x + m_distance * cosf(radAngle) * sinf(radRotation);
    m_camera.position.y = m_camera.target.y + m_distance * sinf(radAngle);
    m_camera.position.z = m_camera.target.z + m_distance * cosf(radAngle) * cosf(radRotation);
} 