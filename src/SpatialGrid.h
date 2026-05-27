#pragma once
#include <vector>
#include "BoidsComponents.h"

class SpatialGrid {
public:
    float cellSize;
    int gridWidth, gridHeight, gridDepth;
    
    std::vector<int> head;
    std::vector<int> next;

    SpatialGrid(float size, int w, int h, int d, int numBoids);

    int getCellIndex(const Vector3& pos) const;

    inline int getCellIndex(int cx, int cy, int cz) const {
        return cx + cy * gridWidth + cz * (gridWidth * gridHeight);
    }

    void build(const BoidsRegistry& registry);
};