#pragma once
#include "BoidsComponents.h"
#include "SpatialGrid.h"

class BoidSystem {
public:
    float viewRadius = 100.0f;
    float cellSize = 100.0f;
    // 排斥距离需要适当增大，因为空间变大了，鸟群的分布也会变得稀疏
    float separationDist = 50.0f;    
    
    // 权重建议
    float alignWeight = 0.5f;
    float cohesionWeight = 0.002f;   // 因为邻居变多了，聚集权重需要极小化
    float separationWeight = 20.0f;  
    
    float minSpeed = 100.0f;
    float maxSpeed = 300.0f; // 大空间下允许更高的飞行速度

    std::vector<Vector3> accelerations;
    BoidSystem(int count) { accelerations.resize(count); }
    void update(BoidsRegistry& reg, SpatialGrid& grid, float dt);
};