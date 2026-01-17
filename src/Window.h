#pragma once 
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window{
public:
    Window(int w, int h, const char* title);
    ~Window();

    bool shouldClose() const;
    void pollEvents();

    GLFWwindow* get() const;
private:
    GLFWwindow* window = nullptr;
};
