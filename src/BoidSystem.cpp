#include "BoidSystem.h"
#include <cmath>
#include <omp.h> 

void BoidSystem::update(BoidsRegistry& reg, SpatialGrid& grid, float dt) {
    grid.build(reg);

    float viewRadSq = viewRadius * viewRadius;
    float sepRadSq = separationDist * separationDist;

    std::fill(accelerations.begin(), accelerations.end(), Vector3{0.0f, 0.0f, 0.0f});

    #pragma omp parallel for
    for (int i = 0; i < reg.count; ++i) {
        Vector3 myPos = reg.positions[i];
        Vector3 align = {0.0f, 0.0f, 0.0f};
        Vector3 coh = {0.0f, 0.0f, 0.0f};
        Vector3 sep = {0.0f, 0.0f, 0.0f};
        int neighbors = 0;

        int cx = static_cast<int>(myPos.x / grid.cellSize);
        int cy = static_cast<int>(myPos.y / grid.cellSize);
        int cz = static_cast<int>(myPos.z / grid.cellSize);

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
                    
                    // 【核心修改 1】加一把绝对性能锁 (checks < 80)
                    // 彻底解决拥挤时的 O(N^2) 性能暴跌问题！
                    int checks = 0;
                    while (neighborId != -1 && checks < 80) {
                        checks++;
                        if (neighborId != i) { 
                            Vector3 nPos = reg.positions[neighborId];
                            Vector3 nVel = reg.velocities[neighborId];
                            
                            float dx = nPos.x - myPos.x;
                            float dy = nPos.y - myPos.y;
                            float dz = nPos.z - myPos.z;
                            float distSq = dx*dx + dy*dy + dz*dz;

                            if (distSq < viewRadSq) {
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

            Vector3 center = {5000.0f, 5000.0f, 5000.0f};
            Vector3 toCenter = {center.x - myPos.x, center.y - myPos.y, center.z - myPos.z};
            
            // 计算到中心的绝对距离
            float distToCenter = std::sqrt(toCenter.x*toCenter.x + toCenter.y*toCenter.y + toCenter.z*toCenter.z);
            
            // 将向心向量归一化 (Normalize)！
            // 这是撑开漩涡的魔法：让引力变成方向，而不是一个巨大无比的数值。
            if (distToCenter > 0.0001f) {
                toCenter.x /= distToCenter;
                toCenter.y /= distToCenter;
                toCenter.z /= distToCenter;
            }
            
            Vector3 swirl = { -toCenter.y, toCenter.x, 0.0f }; 
            // swirl 也必须归一化，保证旋转力纯粹
            float swirlMag = std::sqrt(swirl.x*swirl.x + swirl.y*swirl.y);
            if (swirlMag > 0.0001f) { swirl.x /= swirlMag; swirl.y /= swirlMag; }

            // 重新平衡：给一个恒定的拉力和一个巨大的旋转力
            float centerStrength = 200.0f;  // 温和地向内拉扯
            float swirlStrength = 400.0f;   // 极强地向侧面甩（离心力）

            accelerations[i].x += (toCenter.x * centerStrength) + (swirl.x * swirlStrength);
            accelerations[i].y += (toCenter.y * centerStrength) + (swirl.y * swirlStrength);
            
            // Z 轴给一个稍小的拉力，让漩涡在上下方向呈现出立体的厚度
            accelerations[i].z += (toCenter.z * 50.0f); 

            // ... 后面加上 flocking 力的代码保持不变 ...
            accelerations[i].x += align.x + coh.x + sep.x;
            accelerations[i].y += align.y + coh.y + sep.y;
            accelerations[i].z += align.z + coh.z + sep.z;
        }
    }

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

        auto wrap = [](float& pos, float min, float max) {
            if (pos < min) pos = max;
            else if (pos > max) pos = min;
        };

        // 【核心修改 4】把物理限制墙推远到 10000.0f
        wrap(reg.positions[i].x, 0.0f, 10000.0f);
        wrap(reg.positions[i].y, 0.0f, 10000.0f);
        wrap(reg.positions[i].z, 0.0f, 10000.0f);
    }
}