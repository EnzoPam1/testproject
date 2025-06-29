#include <cmath>
#include "Tile.hpp"

Tile::Tile(int x, int y, float tileSize) : m_x(x), m_y(y), m_tileSize(tileSize) {
    m_worldPosition = { x * m_tileSize, 0, y * m_tileSize };
}

void Tile::SetData(const TileData& data) {
    m_data = data;
}

void Tile::Draw(Model& tileModel, Texture2D& texture) {
    // Dessiner la tuile avec une hauteur très faible pour le sol
    Color tileColor = ((m_x + m_y) % 2 == 0) ? Color{34, 139, 34, 255} : Color{46, 125, 50, 255};

    // Position centrée pour la tuile
    Vector3 tilePos = { m_worldPosition.x + m_tileSize * 0.5f, 0, m_worldPosition.z + m_tileSize * 0.5f };

    // Dessiner un cube plat pour le sol
    DrawCube(tilePos, m_tileSize * 0.98f, 0.1f, m_tileSize * 0.98f, tileColor);

    // Bordures de la tuile
    DrawCubeWires(tilePos, m_tileSize, 0.12f, m_tileSize, Color{0, 0, 0, 100});
}
void Tile::DrawResources(Model resourceModels[7], Color resourceColors[7]) {
    // Affiche chaque ressource présente sur la tuile
    float angleStep = 2 * PI / 7.0f;
    float radius = m_tileSize * 0.3f;
    float baseY = m_resourceHeight;
    for (int i = 0; i < 7; ++i) {
        int count = (i == 0) ? m_data.food : m_data.stones[i-1];
        for (int j = 0; j < count; ++j) {
            float angle = i * angleStep + j * 0.15f;
            float x = m_worldPosition.x + cosf(angle) * radius;
            float y = m_worldPosition.y + baseY + 0.2f * j;
            float z = m_worldPosition.z + sinf(angle) * radius;
            Vector3 pos = {x, y, z};
            DrawModel(resourceModels[i], pos, 0.3f, resourceColors[i]);
        }
    }
}
