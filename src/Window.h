#pragma once 
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <utility>

class Window{
public:
    Window(int w, int h, const char* title);
    ~Window();
    
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    bool shouldClose() const;
    void pollEvents();

    GLFWwindow* get() const;

    // returns framebuffer size in pixels. Never Negative.
    std::pair<int, int> framebufferSize() const;
private:
    GLFWwindow* window = nullptr;
};
