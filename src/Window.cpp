#include "Window.h"
#include <stdexcept>

Window::Window(int w, int h, const char* title) {
    if (!glfwInit()) {
        throw std::runtime_error("GLFW init failed");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window) {
        throw std::runtime_error("Window creation failed");
    }
}

Window::~Window() {
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

GLFWwindow* Window::get() const {
    return window;
}
