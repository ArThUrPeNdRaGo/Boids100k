#include "BoidSystem.h"
#include <cmath>
#include <omp.h> // 【新增】引入 OpenMP 库

void BoidSystem::update(BoidsRegistry& reg, SpatialGrid& grid, float dt) {
    grid.build(reg);

    float viewRadSq = viewRadius * viewRadius;
    float sepRadSq = separationDist * separationDist;

    std::fill(accelerations.begin(), accelerations.end(), Vector3{0.0f, 0.0f, 0.0f});

    // 【核心大招】添加这一行，告诉编译器用所有核心并行计算这个循环
    #pragma omp parallel for
    for (int i = 0; i < reg.count; ++i) {
        Vector3 myPos = reg.positions[i];
        
        Vector3 align = {0.0f, 0.0f, 0.0f};
        Vector3 coh = {0.0f, 0.0f, 0.0f};
        Vector3 sep = {0.0f, 0.0f, 0.0f};
        int neighbors = 0;

        // 1. 手动计算当前鸟在网格中的 3D 坐标 (cx, cy, cz)
        // 注意：这里不用 clamp，因为我们需要真实的坐标来进行偏移计算
        int cx = static_cast<int>(myPos.x / grid.cellSize);
        int cy = static_cast<int>(myPos.y / grid.cellSize);
        int cz = static_cast<int>(myPos.z / grid.cellSize);

        // 2. 核心大招：遍历周围 27 个格子 (包括自己)
        for (int zOffset = -1; zOffset <= 1; ++zOffset) {
            for (int yOffset = -1; yOffset <= 1; ++yOffset) {
                for (int xOffset = -1; xOffset <= 1; ++xOffset) {
                    
                    // 算出当前正在检查的邻居格子的 3D 坐标
                    int nx = cx + xOffset;
                    int ny = cy + yOffset;
                    int nz = cz + zOffset;

                    // 3. 安全锁（极其重要）：防止找到地图外面去，导致内存越界崩溃
                    if (nx < 0 || nx >= grid.gridWidth ||
                        ny < 0 || ny >= grid.gridHeight ||
                        nz < 0 || nz >= grid.gridDepth) {
                        continue; // 如果越界了，直接跳过这个无效格子
                    }

                    // 4. 降维打击：把 3D 坐标压平，算出它在你那个 head 一维数组里的具体位置
                    int neighborCellIdx = grid.getCellIndex(nx, ny, nz); 
                    
                    // 5. 拿到这个邻居格子的链表头！
                    int neighborId = grid.head[neighborCellIdx];
                    // 6. 顺藤摸瓜，把你原来的物理判断逻辑放进来
                    while (neighborId != -1) {
                        if (neighborId != i) { // 依然要防止自己跟自己算
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
                                        // 【防重叠补丁】如果两只鸟完全重合，强行给一个推力推开
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
                    } // end of while
                }
            }
        }

        if (neighbors > 0) {
            Vector3 myVel = reg.velocities[i]; // 获取我当前的速度

            // 1. 对齐 (Alignment)：平均速度 - 自身速度
            align.x = ((align.x / neighbors) - myVel.x) * alignWeight;
            align.y = ((align.y / neighbors) - myVel.y) * alignWeight;
            align.z = ((align.z / neighbors) - myVel.z) * alignWeight;

            // 2. 聚集 (Cohesion)：平均位置 - 自身位置
            coh.x = ((coh.x / neighbors) - myPos.x) * cohesionWeight;
            coh.y = ((coh.y / neighbors) - myPos.y) * cohesionWeight;
            coh.z = ((coh.z / neighbors) - myPos.z) * cohesionWeight;

            // 3. 分离 (Separation)：直接乘以超大权重
            sep.x *= separationWeight; 
            sep.y *= separationWeight; 
            sep.z *= separationWeight;

            // --- 【新增】3D 旋涡力场 ---
            Vector3 center = {500.0f, 500.0f, 500.0f};
            Vector3 toCenter = {center.x - myPos.x, center.y - myPos.y, center.z - myPos.z};
            
            // 叉乘计算切向力：这会让鸟不仅仅飞向中心，而是绕着中心转圈
            Vector3 swirl = { -toCenter.y, toCenter.x, 0.0f }; 
            float swirlMag = std::sqrt(swirl.x*swirl.x + swirl.y*swirl.y);
            if (swirlMag > 0.0001f) { swirl.x /= swirlMag; swirl.y /= swirlMag; }

            // 物理控制
            float centerStrength = 0.1f; // 适度的拉力
            float swirlStrength = 15.0f; // 旋涡力度

            accelerations[i].x += (toCenter.x * centerStrength) + (swirl.x * swirlStrength);
            accelerations[i].y += (toCenter.y * centerStrength) + (swirl.y * swirlStrength);
            accelerations[i].z += (toCenter.z * centerStrength); // 保持聚集

            // 把之前的 flocking 力加上
            accelerations[i].x += align.x + coh.x + sep.x;
            accelerations[i].y += align.y + coh.y + sep.y;
            accelerations[i].z += align.z + coh.z + sep.z;

            float noise = (float)(i % 100) / 50.0f; 
            accelerations[i].x += (std::sin(noise + dt) * 0.5f);
            accelerations[i].y += (std::cos(noise + dt) * 0.5f);
        }
    }

    for (int i = 0; i < reg.count; ++i) {
        reg.velocities[i].x += accelerations[i].x * dt;
        reg.velocities[i].y += accelerations[i].y * dt;
        reg.velocities[i].z += accelerations[i].z * dt;

        float speed = std::sqrt(reg.velocities[i].x*reg.velocities[i].x + 
                                reg.velocities[i].y*reg.velocities[i].y + 
                                reg.velocities[i].z*reg.velocities[i].z);
        
        // 【防除以零安全锁，并执行双向限幅】
        if (speed > 0.0001f) {
            if (speed > maxSpeed) {
                // 如果超速，踩刹车
                reg.velocities[i].x = (reg.velocities[i].x / speed) * maxSpeed;
                reg.velocities[i].y = (reg.velocities[i].y / speed) * maxSpeed;
                reg.velocities[i].z = (reg.velocities[i].z / speed) * maxSpeed;
            } else if (speed < minSpeed) {
                // 如果太慢了，强制踩油门，打破死锁！
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

        wrap(reg.positions[i].x, 0.0f, 1000.0f);
        wrap(reg.positions[i].y, 0.0f, 1000.0f);
        wrap(reg.positions[i].z, 0.0f, 1000.0f);
    }
}