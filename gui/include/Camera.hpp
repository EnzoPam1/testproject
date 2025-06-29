/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
**
*/

#pragma once

#include "raylib.h"
#include "raymath.h"

class GameCamera {
public:
    GameCamera(int screenWidth, int screenHeight);
    ~GameCamera() = default;

    void Update(float deltaTime);
    void BeginMode3D();
    void EndMode3D();

    void SetTarget(Vector3 target);
    void SetBounds(float minX, float maxX, float minZ, float maxZ);

    Camera3D& GetCamera() { return m_camera; }
    Ray GetMouseRay();

private:
    Camera3D m_camera;

    float m_distance = 60.0f;
    float m_angle = 45.0f;
    float m_rotation = 45.0f;
    float m_minDistance = 8.0f;
    float m_maxDistance = 200.0f;

    float m_minX = -50.0f;
    float m_maxX = 50.0f;
    float m_minZ = -50.0f;
    float m_maxZ = 50.0f;

    Vector3 m_targetPosition;
    float m_smoothSpeed = 8.0f;

    float m_rotationSpeed = 90.0f;
    float m_panSpeed = 25.0f;
    float m_zoomSpeed = 8.0f;

    void HandleKeyboardInput(float deltaTime);
    void HandleMouseInput();
    void UpdateCameraPosition();
};
