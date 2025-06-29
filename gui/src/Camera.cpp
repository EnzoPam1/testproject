/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
**
*/

#include "Camera.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

GameCamera::GameCamera(int screenWidth, int screenHeight) {
    m_camera.position = { 40.0f, 40.0f, 40.0f };
    m_camera.target = { 0.0f, 0.0f, 0.0f };
    m_camera.up = { 0.0f, 1.0f, 0.0f };
    m_camera.fovy = 45.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;

    m_targetPosition = m_camera.target;
}

void GameCamera::Update(float deltaTime) {
    HandleKeyboardInput(deltaTime);
    HandleMouseInput();

    m_camera.target.x += (m_targetPosition.x - m_camera.target.x) * m_smoothSpeed * deltaTime;
    m_camera.target.z += (m_targetPosition.z - m_camera.target.z) * m_smoothSpeed * deltaTime;

    UpdateCameraPosition();
}

void GameCamera::HandleKeyboardInput(float deltaTime) {
    float moveSpeed = m_panSpeed * deltaTime;
    float rotSpeed = m_rotationSpeed * deltaTime;

    float radRotation = m_rotation * DEG2RAD;
    Vector3 forward = { sinf(radRotation), 0, cosf(radRotation) };
    Vector3 right = { cosf(radRotation), 0, -sinf(radRotation) };

    if (IsKeyDown(KEY_W)) { // Z = Forward
        m_targetPosition.x -= forward.x * moveSpeed;
        m_targetPosition.z -= forward.z * moveSpeed;
    }
    if (IsKeyDown(KEY_S)) { // S = Backward
        m_targetPosition.x += forward.x * moveSpeed;
        m_targetPosition.z += forward.z * moveSpeed;
    }
    if (IsKeyDown(KEY_A)) { // Q = Left
        m_targetPosition.x -= right.x * moveSpeed;
        m_targetPosition.z -= right.z * moveSpeed;
    }
    if (IsKeyDown(KEY_D)) { // D = Right
        m_targetPosition.x += right.x * moveSpeed;
        m_targetPosition.z += right.z * moveSpeed;
    }

    if (IsKeyDown(KEY_UP)) {
        m_targetPosition.x -= forward.x * moveSpeed;
        m_targetPosition.z -= forward.z * moveSpeed;
    }
    if (IsKeyDown(KEY_DOWN)) {
        m_targetPosition.x += forward.x * moveSpeed;
        m_targetPosition.z += forward.z * moveSpeed;
    }
    if (IsKeyDown(KEY_LEFT)) {
        m_targetPosition.x -= right.x * moveSpeed;
        m_targetPosition.z -= right.z * moveSpeed;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        m_targetPosition.x += right.x * moveSpeed;
        m_targetPosition.z += right.z * moveSpeed;
    }

    if (IsKeyDown(KEY_Q)) { // A = Rotate left
        m_rotation -= rotSpeed;
    }
    if (IsKeyDown(KEY_E)) { // E = Rotate right
        m_rotation += rotSpeed;
    }

    if (IsKeyPressed(KEY_ONE)) {   // & = North view
        m_rotation = 0.0f;
    }
    if (IsKeyPressed(KEY_TWO)) {   // é = East view
        m_rotation = 90.0f;
    }
    if (IsKeyPressed(KEY_THREE)) { // " = South view
        m_rotation = 180.0f;
    }
    if (IsKeyPressed(KEY_FOUR)) {  // ' = West view
        m_rotation = 270.0f;
    }

    // R to reset camera
    if (IsKeyPressed(KEY_R)) {
        m_targetPosition.x = 0.0f;
        m_targetPosition.z = 0.0f;
        m_rotation = 45.0f;
        m_distance = 60.0f;
        m_angle = 45.0f;
    }

    m_targetPosition.x = std::clamp(m_targetPosition.x, m_minX, m_maxX);
    m_targetPosition.z = std::clamp(m_targetPosition.z, m_minZ, m_maxZ);
}

void GameCamera::HandleMouseInput() {
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        m_distance -= wheel * m_zoomSpeed;
        m_distance = std::clamp(m_distance, m_minDistance, m_maxDistance);
    }
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

void GameCamera::UpdateCameraPosition() {
    float radAngle = m_angle * DEG2RAD;
    float radRotation = m_rotation * DEG2RAD;

    m_camera.position.x = m_camera.target.x + m_distance * cosf(radAngle) * sinf(radRotation);
    m_camera.position.y = m_camera.target.y + m_distance * sinf(radAngle);
    m_camera.position.z = m_camera.target.z + m_distance * cosf(radAngle) * cosf(radRotation);
}
