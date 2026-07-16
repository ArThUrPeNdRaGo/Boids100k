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
    
    // grid size
    SpatialGrid grid(100.0f, 100, 100, 100, registry.count);
    BoidSystem system(registry.count);

    std::mt19937 rng(42);

    std::uniform_real_distribution<float> posDist(0.0f, 10000.0f);
    std::uniform_real_distribution<float> velDist(-150.0f, 150.0f);

    for (int i = 0; i < registry.count; ++i) {
        registry.positions[i] = {posDist(rng), posDist(rng), posDist(rng)};
        registry.velocities[i] = {velDist(rng), velDist(rng), velDist(rng)};
    }

    std::cout << "Starting Main Loop (Rendering)..." << std::endl;
    
    Renderer renderer;
    renderer.init();

    // 【last frame
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!renderer.shouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        auto t1 = std::chrono::high_resolution_clock::now();
        system.update(registry, grid, deltaTime);
        auto t2 = std::chrono::high_resolution_clock::now();

        renderer.drawInstanced(registry.positions, registry.velocities);
        auto t3 = std::chrono::high_resolution_clock::now();

        double logicMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
        double renderMs = std::chrono::duration<double, std::milli>(t3 - t2).count();
        double totalMs = logicMs + renderMs;

        std::cout << "\rLogic: " << logicMs << " ms | Render: " << renderMs << " ms    " << std::flush;

        // precise sleep(after logic and render) to maintain ~90 FPS
        double targetFrameTimeMs = 11.111;
        double sleepTimeMs = targetFrameTimeMs - totalMs;
        if (sleepTimeMs > 0.0) {
            std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(sleepTimeMs * 1000.0)));
        }
    }

    return 0;
}