#include "BoidSystem.h"
#include <cmath>
#include <omp.h> 

void BoidSystem::update(BoidsRegistry& reg, SpatialGrid& grid, float dt) {
    // 1. 建立网格
    grid.build(reg);

    float viewRadSq = viewRadius * viewRadius;
    float sepRadSq = separationDist * separationDist;

    // 2. 第一步：多线程并行计算所有鸟的加速度（只读 positions 和 velocities）
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < reg.count; ++i) {
        Vector3 myPos = reg.positions[i];
        Vector3 align = {0.0f, 0.0f, 0.0f};
        Vector3 coh = {0.0f, 0.0f, 0.0f};
        Vector3 sep = {0.0f, 0.0f, 0.0f};
        int neighbors = 0;

        int cx = static_cast<int>(myPos.x / grid.cellSize);
        int cy = static_cast<int>(myPos.y / grid.cellSize);
        int cz = static_cast<int>(myPos.z / grid.cellSize);

        // 阈值控制：整体限制总查询数，防止 27 个格子撑爆计算
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
                    
                    while (neighborId != -1) {
                        totalChecks++;
                        if (totalChecks >= 30) { // 【大幅收紧】改为 30 次，对于 10 万只鸟足够了
                            goto BREAK_NEIGHBOR_LOOPS; // 真正的一箭穿心，彻底跳出 3 层循环
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
                                neighbors++;
                            }
                        }
                        neighborId = grid.next[neighborId];
                    } 
                }
            }
        }
        BREAK_NEIGHBOR_LOOPS:;

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

        // 星系引力中心逻辑
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

    // 3. 第二步：多线程并行应用位移（没有任何读写冲突，速度极快）
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

        // 边界包抄
        if (reg.positions[i].x < 0.0f) reg.positions[i].x = 10000.0f;
        else if (reg.positions[i].x > 10000.0f) reg.positions[i].x = 0.0f;

        if (reg.positions[i].y < 0.0f) reg.positions[i].y = 10000.0f;
        else if (reg.positions[i].y > 10000.0f) reg.positions[i].y = 0.0f;

        if (reg.positions[i].z < 0.0f) reg.positions[i].z = 10000.0f;
        else if (reg.positions[i].z > 10000.0f) reg.positions[i].z = 0.0f;
    }
}