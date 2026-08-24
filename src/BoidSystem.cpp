#include "BoidSystem.h"
#include <cmath>
#include <omp.h> 
#include <cstdio>
#include <atomic>

void BoidSystem::update(BoidsRegistry& reg, SpatialGrid& grid, float dt) {
    // create grid
    double gridStartTime = omp_get_wtime();
    grid.build(reg);
    double gridBuildTime = (omp_get_wtime() - gridStartTime) * 1000.0;

    float viewRadSq = viewRadius * viewRadius;
    float sepRadSq = separationDist * separationDist;

    int chunckSize = 128;
    int totalChunks = (reg.count + chunckSize - 1) / chunckSize;
    std::atomic<int> nextTaskId{0};

    long long globalTotalChecks = 0; 
    
    double physicsStartTime = omp_get_wtime();

    int maxThreads = omp_get_max_threads();
    std::vector<double> threadWorkTimes(maxThreads, 0.0);

    #pragma omp parallel
    {
        int threadId = omp_get_thread_num();
        double threadStartTime = omp_get_wtime(); // Start time for this thread

        long long localTotalChecks = 0; // Local variable for each thread
        int taskIdx;

        while((taskIdx = nextTaskId.fetch_add(1, std::memory_order_relaxed)) < totalChunks){
            int startIdx = taskIdx * chunckSize;
            int endIdx = std::min(startIdx + chunckSize, reg.count);

            for (int i = startIdx; i < endIdx; ++i) {
                Vector3 myPos = reg.positions[i];
                Vector3 align = {0.0f, 0.0f, 0.0f};
                Vector3 coh = {0.0f, 0.0f, 0.0f};
                Vector3 sep = {0.0f, 0.0f, 0.0f};
                int neighbors = 0;

                int cx = static_cast<int>(myPos.x / grid.cellSize);
                int cy = static_cast<int>(myPos.y / grid.cellSize);
                int cz = static_cast<int>(myPos.z / grid.cellSize);

                int totalChecks = 0;

                for (int zOffset = -1; zOffset <= 1; ++zOffset) {
                    for (int yOffset = -1; yOffset <= 1; ++yOffset) {
                        for (int xOffset = -1; xOffset <= 1; ++xOffset) {
                            
                            int nx = cx + xOffset;
                            int ny = cy + yOffset;
                            int nz = cz + zOffset;

                            if (nx < 0 || nx >= grid.gridWidth ||
                                ny < 0 || ny >= grid.gridHeight ||
                                nz < 0 || nz >= grid.gridDepth) {
                                continue; 
                            }

                            int neighborCellIdx = grid.getCellIndex(nx, ny, nz); 
                            int neighborId = grid.head[neighborCellIdx];
                            
                            int cellChecks = 0; 

                            while (neighborId != -1) {
                                totalChecks++;
                                if (cellChecks > 4) { 
                                    break; 
                                }

                                if (neighborId != i) { 
                                    Vector3 nPos = reg.positions[neighborId];
                                    
                                    float dx = nPos.x - myPos.x;
                                    float dy = nPos.y - myPos.y;
                                    float dz = nPos.z - myPos.z;
                                    float distSq = dx*dx + dy*dy + dz*dz;

                                    if (distSq < viewRadSq) {
                                        Vector3 nVel = reg.velocities[neighborId];
                                        align.x += nVel.x; align.y += nVel.y; align.z += nVel.z;
                                        coh.x += nPos.x; coh.y += nPos.y; coh.z += nPos.z;

                                        if (distSq < sepRadSq) {
                                            if (distSq < 0.0001f) {
                                                sep.x += 1.0f; sep.y -= 1.0f; 
                                            } else {
                                                float force = 1.0f / (distSq + 0.01f); 
                                                sep.x -= dx * force * 2.0f;
                                                sep.y -= dy * force * 2.0f;
                                                sep.z -= dz * force * 2.0f;
                                            }
                                        }
                                        cellChecks++;
                                        neighbors++;

                                        if (neighbors >= 30) {
                                            goto BREAK_NEIGHBOR_LOOPS;
                                        }
                                    }
                                }
                                neighborId = grid.next[neighborId];
                            } 
                        }
                    }
                }
                BREAK_NEIGHBOR_LOOPS:;

                localTotalChecks += totalChecks;

                Vector3 acc = {0.0f, 0.0f, 0.0f};

                if (neighbors > 0) {
                    Vector3 myVel = reg.velocities[i]; 

                    align.x = ((align.x / neighbors) - myVel.x) * alignWeight;
                    align.y = ((align.y / neighbors) - myVel.y) * alignWeight;
                    align.z = ((align.z / neighbors) - myVel.z) * alignWeight;

                    coh.x = ((coh.x / neighbors) - myPos.x) * cohesionWeight;
                    coh.y = ((coh.y / neighbors) - myPos.y) * cohesionWeight;
                    coh.z = ((coh.z / neighbors) - myPos.z) * cohesionWeight;

                    sep.x *= separationWeight; 
                    sep.y *= separationWeight; 
                    sep.z *= separationWeight;

                    acc.x += align.x + coh.x + sep.x;
                    acc.y += align.y + coh.y + sep.y;
                    acc.z += align.z + coh.z + sep.z;
                }

                // Boids pattern: swirling towards the center of the space
                Vector3 center = {5000.0f, 5000.0f, 5000.0f};
                Vector3 toCenter = {center.x - myPos.x, center.y - myPos.y, center.z - myPos.z};
                float distToCenter = std::sqrt(toCenter.x*toCenter.x + toCenter.y*toCenter.y + toCenter.z*toCenter.z);
                
                if (distToCenter > 0.0001f) {
                    toCenter.x /= distToCenter; toCenter.y /= distToCenter; toCenter.z /= distToCenter;
                    
                    Vector3 swirl = { -toCenter.y, toCenter.x, 0.0f }; 
                    float swirlMag = std::sqrt(swirl.x*swirl.x + swirl.y*swirl.y);
                    if (swirlMag > 0.0001f) { swirl.x /= swirlMag; swirl.y /= swirlMag; }

                    acc.x += (toCenter.x * 200.0f) + (swirl.x * 400.0f);
                    acc.y += (toCenter.y * 200.0f) + (swirl.y * 400.0f);
                    acc.z += (toCenter.z * 50.0f); 
                }

                accelerations[i] = acc;
            }

        }

        threadWorkTimes[threadId] = (omp_get_wtime() - threadStartTime) * 1000.0;
        #pragma omp atomic
        globalTotalChecks += localTotalChecks;
    }

    
    double physicsTime = (omp_get_wtime() - physicsStartTime) * 1000.0;

    //multi tasking: update velocity and position
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < reg.count; ++i) {
        reg.velocities[i].x += accelerations[i].x * dt;
        reg.velocities[i].y += accelerations[i].y * dt;
        reg.velocities[i].z += accelerations[i].z * dt;

        float speed = std::sqrt(reg.velocities[i].x*reg.velocities[i].x + 
                                reg.velocities[i].y*reg.velocities[i].y + 
                                reg.velocities[i].z*reg.velocities[i].z);
        
        if (speed > 0.0001f) {
            if (speed > maxSpeed) {
                reg.velocities[i].x = (reg.velocities[i].x / speed) * maxSpeed;
                reg.velocities[i].y = (reg.velocities[i].y / speed) * maxSpeed;
                reg.velocities[i].z = (reg.velocities[i].z / speed) * maxSpeed;
            } else if (speed < minSpeed) {
                reg.velocities[i].x = (reg.velocities[i].x / speed) * minSpeed;
                reg.velocities[i].y = (reg.velocities[i].y / speed) * minSpeed;
                reg.velocities[i].z = (reg.velocities[i].z / speed) * minSpeed;
            }
        }

        reg.positions[i].x += reg.velocities[i].x * dt;
        reg.positions[i].y += reg.velocities[i].y * dt;
        reg.positions[i].z += reg.velocities[i].z * dt;

        // boundary case
        if (reg.positions[i].x < 0.0f) reg.positions[i].x = 10000.0f;
        else if (reg.positions[i].x > 10000.0f) reg.positions[i].x = 0.0f;

        if (reg.positions[i].y < 0.0f) reg.positions[i].y = 10000.0f;
        else if (reg.positions[i].y > 10000.0f) reg.positions[i].y = 0.0f;

        if (reg.positions[i].z < 0.0f) reg.positions[i].z = 10000.0f;
        else if (reg.positions[i].z > 10000.0f) reg.positions[i].z = 0.0f;
    }

    static int frameCounter = 0;
    if (++frameCounter % 60 == 0) {
        // find max boids in one cell
        int maxBoidsInOneCell = 0;
        for (int c = 0; c < grid.gridWidth * grid.gridHeight * grid.gridDepth; ++c) {
            int count = 0;
            int curr = grid.head[c];
            while (curr != -1) {
                count++;
                curr = grid.next[curr];
            }
            if (count > maxBoidsInOneCell) maxBoidsInOneCell = count;
        }

        printf("\n--- Profiling Data ---\n");
        printf("Grid Build: %.3f ms | Physics: %.3f ms\n", gridBuildTime, physicsTime);
        printf("Max Cell Density: %d boids in one cell\n", maxBoidsInOneCell);
        printf("Global Checks : %lld (Max theoretical: 3,000,000)\n", globalTotalChecks);
        printf("----------------------\n");

        printf("--- Thread Load ---\n");
        for(int i = 0; i < maxThreads; ++i) {
        printf("Thread %d active time: %.3f ms\n", i, threadWorkTimes[i]);
}
    }
}