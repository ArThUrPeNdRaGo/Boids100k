#pragma once
#include <vector>
#include "BoidsComponents.h"

// 注意：glad 必须在 glfw 之前引入！
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Renderer {
public:
    Renderer();
    ~Renderer();

    void init();
    bool shouldClose() const;
    void drawInstanced(const std::vector<Vector3>& positions, const std::vector<Vector3>& velocities);

private:
    GLFWwindow* window;
    unsigned int shaderProgram;
    unsigned int VAO, VBO_bird, VBO_instance, VBO_vel;
};