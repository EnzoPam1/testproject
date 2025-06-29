#pragma once

#include "raylib.h"
#include <array>

struct TileData {
    int food = 0;
    int stones[6] = {0};
};

class Tile {
public:
    Tile(int x, int y, float tileSize);
    ~Tile() = default;

    void SetData(const TileData& data);
    void Draw(Model& tileModel, Texture2D& texture);
    void DrawResources(Model resourceModels[7], Color resourceColors[7]);

    const TileData& GetData() const { return m_data; }
    Vector3 GetWorldPosition() const { return m_worldPosition; }

private:
    int m_x, m_y;
    TileData m_data;
    Vector3 m_worldPosition;
    
    // Visual properties
    float m_tileSize = 2.0f;
    float m_resourceHeight = 0.1f;
    float m_resourceSpacing = 0.3f;
}; 