#include <iostream>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include "text_renderer.hpp"


namespace {
    const int WIN_WIDTH = 960;
    const int WIN_HEIGHT = 540;
}

void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        std::cout << "Usage: " << argv[0] << " <font file>\n";
        return 1;
    }

    // Initialize GLFW library
    if(glfwInit() != GL_TRUE) {
        std::cerr << "Unable to initialize GLFW library\n";
        return 2;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW window and text renderer
    GLFWwindow *window = glfwCreateWindow(
        WIN_WIDTH, WIN_HEIGHT, "VOGL Example", 0, 0);
    glfwSetKeyCallback(window, &key_callback);
    glfwMakeContextCurrent(window);

    vogl::TextRenderer textRenderer(argv[1], 50);
    textRenderer.setScale(2.f / WIN_WIDTH, 2.f / WIN_HEIGHT);

    // Initialize OpenGL state
    glClearColor(0.95f, 0.95f, 0.95f, 1.f);
    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

    const vogl::Color BLACK(0.f, 0.f, 0.f, 1.f);
    const vogl::Color RED(1.f, 0.f, 0.f, 1.f);
    const vogl::Color GREEN(0.f, 1.f, 0.f, 1.f);
    const vogl::Color BLUE(0.f, 0.f, 1.f, 1.f);

    // Render loop
    while(glfwWindowShouldClose(window) != GL_TRUE) {
        glClear(GL_COLOR_BUFFER_BIT);
        {
            textRenderer.bind();
            textRenderer.drawText("Hello, World!", -0.95f, 0.8f, BLACK);
            textRenderer.drawText("Hello, World!", -0.6f, 0.3f, RED);
            textRenderer.drawText("Hello, World!", -0.2f, -0.3f, GREEN);
            textRenderer.drawText("Hello, World!", 0.2f, -0.95f, BLUE);
            textRenderer.unbind();
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}