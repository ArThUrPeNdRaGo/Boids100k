#include <iostream>
#include <chrono>
#include <random>
#include "BoidsComponents.h"
#include "SpatialGrid.h"
#include "BoidSystem.h"
#include "Renderer.h"

int main() {
    // 你的 BoidsRegistry 默认就是 10 万个
    std::cout << "Engine Start: Allocating Memory for Boids..." << std::endl;
    
    BoidsRegistry registry; 
    
    // 1. 网格的 Z 轴深度改成 1 (因为是 2D 世界了)
    SpatialGrid grid(100.0f, 50, 50, 50, registry.count);
    BoidSystem system(registry.count);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> posDist(0.0f, 5000.0f);
    std::uniform_real_distribution<float> velDist(-50.0f, 50.0f);

    for (int i = 0; i < registry.count; ++i) {
        // 2. 恢复 Z 轴的随机生成，解除降维打击
        registry.positions[i] = {posDist(rng), posDist(rng), posDist(rng)};
        registry.velocities[i] = {velDist(rng), velDist(rng), velDist(rng)};
    }

    std::cout << "Starting Main Loop (Rendering)..." << std::endl;
    float deltaTime = 0.016f; // 固定帧率模拟（约 60FPS 的时间步长）

    // 初始化渲染器
    Renderer renderer;
    renderer.init();

    // 真正的游戏死循环
    while (!renderer.shouldClose()) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // 1. CPU 进行 ECS 高性能计算
        system.update(registry, grid, deltaTime);
        
        // 2. GPU 进行一键实例化渲染
        renderer.drawInstanced(registry.positions, registry.velocities);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms = end - start;
        
        // 在终端动态刷新每一帧的耗时 (用 \r 回车不换行)
        std::cout << "\rFrame Calculation + Render Time: " << ms.count() << " ms    " << std::flush;
    }

    return 0;
}