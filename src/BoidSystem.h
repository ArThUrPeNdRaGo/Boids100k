#pragma once
#include "BoidsComponents.h"
#include "SpatialGrid.h"

class BoidSystem {
public:
    float viewRadius = 100.0f;
    float cellSize = 100.0f;
    
    float separationDist = 150.0f;    
    
    float alignWeight = 0.5f;
    float cohesionWeight = 0.5f;     
    
    // 【核心修改 1】将排斥力暴增 10 倍！
    // 像磁铁同极一样，强制它们保持巨大的个人空间，从物理层面杜绝堆积！
    float separationWeight = 300.0f;  
    
    float minSpeed = 300.0f;
    float maxSpeed = 1500.0f; 

    std::vector<Vector3> accelerations;
    BoidSystem(int count) { accelerations.resize(count); }
    void update(BoidsRegistry& reg, SpatialGrid& grid, float dt);
};