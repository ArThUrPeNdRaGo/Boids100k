#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include "BoidsComponents.h"
#include "SpatialGrid.h"
#include "BoidSystem.h"
#include "Renderer.h"

int main() {
    #if defined(_MSC_VER)
    _putenv_s("OMP_WAIT_POLICY", "PASSIVE");
    #else
        setenv("OMP_WAIT_POLICY", "PASSIVE", 1);
    #endif

    std::cout << "Engine Start: Allocating Memory for Boids..." << std::endl;
    
    BoidsRegistry registry; 
    
    // 世界长宽变为 10000。格子大小100，所以网格是 100x100x100
    SpatialGrid grid(100.0f, 100, 100, 100, registry.count);
    BoidSystem system(registry.count);

    std::mt19937 rng(42);
    // 把初始散布范围扩大到 10000.0f，彻底稀释 10 万只鸟的密度
    std::uniform_real_distribution<float> posDist(0.0f, 10000.0f);
    std::uniform_real_distribution<float> velDist(-150.0f, 150.0f);

    for (int i = 0; i < registry.count; ++i) {
        registry.positions[i] = {posDist(rng), posDist(rng), posDist(rng)};
        registry.velocities[i] = {velDist(rng), velDist(rng), velDist(rng)};
    }

    std::cout << "Starting Main Loop (Rendering)..." << std::endl;
    
    Renderer renderer;
    renderer.init();

    // 【新增】记录上一帧的时间点
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!renderer.shouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // --- 1. 测算纯物理与多线程计算耗时 ---
        auto t1 = std::chrono::high_resolution_clock::now();
        system.update(registry, grid, deltaTime);
        auto t2 = std::chrono::high_resolution_clock::now();

        // --- 2. 测算 GPU 数据上传与渲染耗时 ---
        renderer.drawInstanced(registry.positions, registry.velocities);
        auto t3 = std::chrono::high_resolution_clock::now();

        // --- 打印拆解耗时 ---
        double logicMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
        double renderMs = std::chrono::duration<double, std::milli>(t3 - t2).count();
        double totalMs = logicMs + renderMs;

        std::cout << "\rLogic: " << logicMs << " ms | Render: " << renderMs << " ms    " << std::flush;

        // --- 精确休眠 ---
        double targetFrameTimeMs = 11.111;
        double sleepTimeMs = targetFrameTimeMs - totalMs;
        if (sleepTimeMs > 0.0) {
            std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(sleepTimeMs * 1000.0)));
        }
    }

    return 0;
}