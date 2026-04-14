#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

#include "VkPhysicalDevice.h"
#include "VkLogicalDevice.h"

struct Swapchain{
    VkSwapchainKHR handle = VK_NULL_HANDLE;
    VkFormat imageFormat = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D extent = {};
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
};

// Surface
VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window);
void destroySurface(VkInstance instance, VkSurfaceKHR surface);

// Swapchain
Swapchain createSwapchain(
    VkPhysicalDevice physical,
    const LogicalDevice& ld,
    VkSurfaceKHR surface,
    const QueueFamilyIndices& indices,
    GLFWwindow* window,
    VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE
);

void destroySwapchain(VkDevice device, Swapchain& sc);
