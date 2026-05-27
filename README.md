Boids100k: High-Performance ECS Flocking Simulation
Boids100k is a high-performance simulation engine designed to simulate the collective behavior of 100,000+ entities (Boids) in real-time. Built from the ground up using a Data-Oriented Design (DOD) approach, this project leverages the Entity-Component-System (ECS) architecture to maximize CPU cache efficiency and GPU rendering throughput.

🚀 Overview
Traditional object-oriented (OOP) approaches for flocking simulations often face significant performance bottlenecks at scale. This engine addresses these issues by:Decoupling Data and Logic: Using a contiguous structure for entity data to ensure hardware prefetching efficiency.Spatial Partitioning: Implementing a custom Spatial Hashing Grid to reduce the complexity of neighbor lookups from $O(N^2)$ to nearly $O(N)$.Parallel Processing: Leveraging OpenMP to distribute heavy physics computations across all available CPU cores.

🛠️ Core Features
ECS Architecture: Strict separation of entity data (BoidsRegistry) and systems (BoidSystem), ensuring optimal memory layout.Spatial Hashing: Efficiently tracks entity neighbors within local grids to handle massive entity counts without exponential performance degradation.Instanced Rendering: Utilizes OpenGL Instanced Rendering to submit 100,000+ entities with a single Draw Call, minimizing CPU-GPU communication overhead.Frustum Culling: Implements software-level culling to only process and upload visible entities, preserving bandwidth.

📊 Technical Specifications
Entities: 100,000+Architecture: Data-Oriented ECSConcurrency: Multi-threaded via OpenMPTarget Performance: 60 FPS @ < 10ms Frame Time

🏗️ Getting Started
PrerequisitesCMake 3.10+C++17 compatible compilerOpenGL 3.3+ supported graphics hardware
