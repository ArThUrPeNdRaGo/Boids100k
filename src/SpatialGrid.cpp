#include "SpatialGrid.h"
#include <algorithm>

SpatialGrid::SpatialGrid(float size, int w, int h, int d, int numBoids) 
    : cellSize(size), gridWidth(w), gridHeight(h), gridDepth(d) {
    head.assign(w * h * d, -1);
    next.assign(numBoids, -1);
}

int SpatialGrid::getCellIndex(const Vector3& pos) const {
    int cx = std::clamp(static_cast<int>(pos.x / cellSize), 0, gridWidth - 1);
    int cy = std::clamp(static_cast<int>(pos.y / cellSize), 0, gridHeight - 1);
    int cz = std::clamp(static_cast<int>(pos.z / cellSize), 0, gridDepth - 1);
    return cx + cy * gridWidth + cz * (gridWidth * gridHeight);
}

void SpatialGrid::build(const BoidsRegistry& registry) {
    std::fill(head.begin(), head.end(), -1);
    
    for (int i = 0; i < registry.count; ++i) {
        int cellIdx = getCellIndex(registry.positions[i]);
        next[i] = head[cellIdx];
        head[cellIdx] = i;
    }
}