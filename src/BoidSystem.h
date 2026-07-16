#pragma once
#include "BoidsComponents.h"
#include "SpatialGrid.h"

class BoidSystem {
public:
    float viewRadius = 250.0f;
    float cellSize = 100.0f;
    
    float separationDist = 100.0f;    
    
    float alignWeight = 0.5f;
    float cohesionWeight = 0.5f;     
    
    float separationWeight = 300.0f;  
    
    float minSpeed = 300.0f;
    float maxSpeed = 1500.0f; 

    std::vector<Vector3> accelerations;
    BoidSystem(int count) { accelerations.resize(count); }
    void update(BoidsRegistry& reg, SpatialGrid& grid, float dt);
};