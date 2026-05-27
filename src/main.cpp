#include <iostream>
#include <chrono>
#include <random>
#include "BoidsComponents.h"
#include "SpatialGrid.h"
#include "BoidSystem.h"
#include "Renderer.h"

int main() {
    std::cout << "Engine Start: Allocating Memory for Boids..." << std::endl;
    
    BoidsRegistry registry; 
    
    // 【修改 1】世界长宽变为 10000。格子大小100，所以网格是 100x100x100
    SpatialGrid grid(100.0f, 100, 100, 100, registry.count);
    BoidSystem system(registry.count);

    std::mt19937 rng(42);
    // 【修改 2】把初始散布范围扩大到 10000.0f，彻底稀释 10 万只鸟的密度
    std::uniform_real_distribution<float> posDist(0.0f, 10000.0f);
    std::uniform_real_distribution<float> velDist(-150.0f, 150.0f);

    for (int i = 0; i < registry.count; ++i) {
        registry.positions[i] = {posDist(rng), posDist(rng), posDist(rng)};
        registry.velocities[i] = {velDist(rng), velDist(rng), velDist(rng)};
    }

    std::cout << "Starting Main Loop (Rendering)..." << std::endl;
    float deltaTime = 0.016f; 

    Renderer renderer;
    renderer.init();

    while (!renderer.shouldClose()) {
        auto start = std::chrono::high_resolution_clock::now();
        
        system.update(registry, grid, deltaTime);
        renderer.drawInstanced(registry.positions, registry.velocities);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms = end - start;
        std::cout << "\rFrame Calculation + Render Time: " << ms.count() << " ms    " << std::flush;
    }

    return 0;
}