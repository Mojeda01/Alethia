#pragma once

#include <array>
#include <cstdint>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "VkLogicalDevice.h"
#include "VkPhysicalDevice.h"
#include "VkSwapchain.h"
#include "VkPipeline.h"

struct FrameSync{
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence inFlight = VK_NULL_HANDLE;
};

struct DepthResources{
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
};

struct Renderer{
    static constexpr uint32_t kFramesInFlight = 2;
    
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    LogicalDevice logicalDevice{};
    QueueFamilyIndices queueFamilies{};
    
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    GLFWwindow* window = nullptr;
    
    Swapchain swapchain{};
    DepthResources depth{};
    
    Pipeline pipeline{};

    VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, kFramesInFlight> commandBuffers{};
    std::array<FrameSync, kFramesInFlight> frames{};
    
    std::array<VkFence, kFramesInFlight> imageInFlight{};
    uint32_t currentFrame = 0;
    bool framebufferResized = false;
};

Renderer createRenderer(
    VkPhysicalDevice physicalDevice,
    const LogicalDevice& logicalDevice,
    VkSurfaceKHR surface,
    const QueueFamilyIndices& queueFamilies,
    GLFWwindow* window
);

void destroyRenderer(Renderer& renderer);

void drawFrame(Renderer& renderer);

inline void notifyFramebufferResized(Renderer& renderer) {
    renderer.framebufferResized = true;
}
