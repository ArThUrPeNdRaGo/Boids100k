#include "Renderer.h"
#include <iostream>

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aOffset; // 世界坐标 0~1000
    layout (location = 2) in vec3 aVel;

    void main() {
        // 1. 先计算旋转 (保持原样)
        vec3 forward = normalize(aVel);
        vec3 upGuide = vec3(0.0, 1.0, 0.0);
        if (abs(forward.y) > 0.99) upGuide = vec3(1.0, 0.0, 0.0);
        vec3 right = normalize(cross(upGuide, forward));
        vec3 up = cross(forward, right);
        mat3 rot = mat3(right, up, forward);

        // 2. 变换到以世界中心 (500,500,500) 为原点的坐标系 (-500 ~ 500)
        vec3 worldPos = aOffset - vec3(500.0);
        
        // 3. 旋转鸟的形状
        vec3 rotatedPos = rot * (aPos * 10.0); 

        // 4. 【强制显示】直接将坐标归一化到 [-0.9, 0.9]
        // 这一步完全绕开了所有透视除法导致的问题，是目前调试最稳的方案
        vec3 finalPos = (rotatedPos + worldPos) / 600.0; 

        // 输出 (Z轴映射到 gl_Position.z，方便深度测试)
        gl_Position = vec4(finalPos.x, finalPos.y, -worldPos.z / 600.0, 1.0);
    }
)";

// 2. 3D 片段着色器 (加入非常基础的深度雾化伪代码效果，可选但好看)
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        // 利用 gl_FragCoord.z (深度) 让远处的鸟变暗，产生空间纵深感
        float depth = gl_FragCoord.z; 
        FragColor = vec4(0.2, 1.0, 0.8, 0.6);
    }
)";

Renderer::Renderer() : window(nullptr), VAO(0), VBO_bird(0), VBO_instance(0), shaderProgram(0) {}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO_bird);
    glDeleteBuffers(1, &VBO_instance);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
}

void Renderer::init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1000, 1000, "100k Boids ECS - Taiyi", NULL, NULL);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return;

    glEnable(GL_DEPTH_TEST);

    // 3. 编译着色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 4. 定义一只鸟的形状 (一个简单的等腰三角形)
    float birdVertices[] = {
        -0.4f, -0.2f,  0.0f, // 左翼
         0.4f, -0.2f,  0.0f, // 右翼
         0.0f,  0.0f,  0.8f  // 鸟头 (Z方向)
    };

    // 5. 设置 GPU 内存 (VAO, VBO)
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO_bird);
    glGenBuffers(1, &VBO_instance);
    glGenBuffers(1, &VBO_vel);

    // 【重要】一次性绑定所有内容到 VAO
    glBindVertexArray(VAO);

    // 绑定并设置鸟的模型
    glBindBuffer(GL_ARRAY_BUFFER, VBO_bird);
    glBufferData(GL_ARRAY_BUFFER, sizeof(birdVertices), birdVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // 绑定并设置位置
    glBindBuffer(GL_ARRAY_BUFFER, VBO_instance);
    glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
    glVertexAttribDivisor(1, 1); 

    // 绑定并设置速度
    glBindBuffer(GL_ARRAY_BUFFER, VBO_vel);
    glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
    glVertexAttribDivisor(2, 1);

    // 【最后】解绑 VAO
    glBindVertexArray(0); 
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Renderer::drawInstanced(const std::vector<Vector3>& positions, const std::vector<Vector3>& velocities) {
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(Vector3), positions.data());

    glBindBuffer(GL_ARRAY_BUFFER, VBO_vel);
    glBufferSubData(GL_ARRAY_BUFFER, 0, velocities.size() * sizeof(Vector3), velocities.data());

    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, positions.size());
    glfwSwapBuffers(window);
    glfwPollEvents();
}