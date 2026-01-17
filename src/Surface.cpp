#include "Surface.h"
#include <stdexcept>

Surface::Surface(VkInstance inst, GLFWwindow* window)
    : instance(inst)
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Surface creation failed");
    }
}

Surface::~Surface() {
    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
}

VkSurfaceKHR Surface::get() const {
    return surface;
}
