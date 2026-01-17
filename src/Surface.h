#pragma once 
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

class Surface {
public:
    Surface(VkInstance instance, GLFWwindow* window);
    ~Surface();

    VkSurfaceKHR get() const;
private:
    VkInstance instance;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
};
