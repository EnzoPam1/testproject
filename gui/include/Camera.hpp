/*
** EPITECH PROJECT, 2025
** zappy_gui
** File description:
** Auto-chess style camera
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

    // Camera controls
    void SetTarget(Vector3 target);
    void SetBounds(float minX, float maxX, float minZ, float maxZ);
    
    // Getters
    Camera3D& GetCamera() { return m_camera; }
    Ray GetMouseRay();

private:
    Camera3D m_camera;
    
    // Camera parameters - adapted for bigger world scale
    float m_distance = 60.0f;   // Start further back
    float m_angle = 45.0f;      // Isometric angle
    float m_rotation = 45.0f;
    float m_minDistance = 5.0f;  // Beaucoup plus proche pour voir les détails
    float m_maxDistance = 200.0f; // Much further max for overview
    
    // Bounds
    float m_minX = -50.0f;
    float m_maxX = 50.0f;
    float m_minZ = -50.0f;
    float m_maxZ = 50.0f;
    
    // Smooth movement
    Vector3 m_targetPosition;
    float m_smoothSpeed = 5.0f;
    
    // Mouse control
    Vector2 m_lastMousePos;
    bool m_isPanning = false;
    bool m_isRotating = false;
    
    void HandleMouseInput();
    void HandleKeyboardInput(float deltaTime);
    void UpdateCameraPosition();
}; 