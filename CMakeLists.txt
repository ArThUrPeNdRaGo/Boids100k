cmake_minimum_required(VERSION 3.14)
project(Boids100k)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 极致性能优化标志
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -march=native")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /fp:fast")
endif()

# ==========================================
# 1. 自动下载并编译 GLFW
# ==========================================
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ==========================================
# 2. 找到系统自带的 OpenGL
# ==========================================
find_package(OpenGL REQUIRED)

# ==========================================
# 3. 汇总所有你的 C++ 源文件 (注意这里加上了 src/ 前缀)
# ==========================================
add_executable(Boids100k 
    src/main.cpp
    src/BoidSystem.cpp
    src/SpatialGrid.cpp
    src/Renderer.cpp
    src/glad.c
)

# ==========================================
# 4. 把所有零件链接到你的程序上
# ==========================================
# 告诉编译器去哪里找头文件 
# 包含了顶层的 include 目录 (为了找 glad/glad.h)
# 也包含了 src 目录 (为了你的 cpp 文件互相找 .h)
target_include_directories(Boids100k PRIVATE 
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/src
)

# 告诉链接器把 GLFW 和 OpenGL 绑死在你的程序上
target_link_libraries(Boids100k PRIVATE glfw OpenGL::GL)

find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(Boids100k PUBLIC OpenMP::OpenMP_CXX)
endif()