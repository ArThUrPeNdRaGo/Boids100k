#pragma once
#include <vector>

struct Vector3 { float x, y, z; };

struct BoidsRegistry {
    int count = 100000;
    std::vector<Vector3> positions;
    std::vector<Vector3> velocities;
    
    BoidsRegistry() {
        positions.resize(count);
        velocities.resize(count);
    }
};