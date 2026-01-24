#include "Window.h"
#include <stdexcept>

Window::Window(int w, int h, const char* title) {
    if (w <= 0 || h <= 0) {
        throw std::invalid_argument("Window dimensions must be positive.");
    }
    if (title == nullptr || title[0] == '\0') {
        throw std::invalid_argument("Window title must be non-empty");
    }
    if (!glfwInit()) {
        throw std::runtime_error("GLFW init failed");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Window creation failed");
    }
}

Window::~Window() {
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

bool Window::shouldClose() const {
    return window && glfwWindowShouldClose(window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

GLFWwindow* Window::get() const {
    return window;
}

std::pair<int, int> Window::framebufferSize() const {
    int w = 0, h = 0;
    if (window) {
        glfwGetFramebufferSize(window, &w, &h);
    }
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    return {w, h};
}
