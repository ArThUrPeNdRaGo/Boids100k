#include "Renderer.h"
#include <iostream>

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aOffset; 
    layout (location = 2) in vec3 aVel;

    void main() {
        vec3 forward = normalize(aVel);
        vec3 upGuide = vec3(0.0, 1.0, 0.0);
        if (abs(forward.y) > 0.99) upGuide = vec3(1.0, 0.0, 0.0);
        vec3 right = normalize(cross(upGuide, forward));
        vec3 up = cross(forward, right);
        mat3 rot = mat3(right, up, forward);

        // 【摄像机修改】对准世界中心 5000.0
        vec3 worldPos = aOffset - vec3(5000.0);
        vec3 rotatedPos = rot * (aPos * 30.0);

        // 【缩放倍数】调整这里可以拉近/拉远镜头。现在能看到半径为 3500 的区域
        vec3 finalPos = (rotatedPos + worldPos) / 4800.0; 

        gl_Position = vec4(finalPos.x, finalPos.y, -worldPos.z / 10000.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(0.2, 1.0, 0.8, 0.8);
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

    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return;

    glEnable(GL_DEPTH_TEST);

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

    float birdVertices[] = {
        -0.4f, -0.2f,  0.0f, 
         0.4f, -0.2f,  0.0f, 
         0.0f,  0.0f,  0.8f  
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO_bird);
    glGenBuffers(1, &VBO_instance);
    glGenBuffers(1, &VBO_vel);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_bird);
    glBufferData(GL_ARRAY_BUFFER, sizeof(birdVertices), birdVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_instance);
    glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
    glVertexAttribDivisor(1, 1); 

    glBindBuffer(GL_ARRAY_BUFFER, VBO_vel);
    glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
    glVertexAttribDivisor(2, 1);

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

    if (!positions.empty()) {
        // --- 优化1：位置缓冲区孤儿化 ---
        glBindBuffer(GL_ARRAY_BUFFER, VBO_instance);
        // 这一行是关键：给驱动传 nullptr，强制驱动断开连接，开辟新同步区，CPU 瞬间解放！
        glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW); 
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(Vector3), positions.data());

        // --- 优化2：速度缓冲区孤儿化 ---
        glBindBuffer(GL_ARRAY_BUFFER, VBO_vel);
        // 同样，传 nullptr 杜绝 CPU 死等
        glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, velocities.size() * sizeof(Vector3), velocities.data());

        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, static_cast<GLsizei>(positions.size()));
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
}