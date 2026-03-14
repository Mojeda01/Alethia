#pragma once 

#include "Window.h"
#include "Instance.h"
#include "Surface.h"
#include "Device.h"
#include "Swapchain.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "CommandPool.h"
#include "SwapchainBundle.h"
#include "FrameSync.h"
#include "MeshBuffer.h"
#include "triangle/TriangleRenderer.h"

#include <chrono>
#include <cstdint>

class VulkanApp{
public:
    VulkanApp(int width, int height, const char* title);
    VulkanApp(const VulkanApp&) = delete;
    VulkanApp& operator=(const VulkanApp&) = delete;
    void run();
private:
    void initVulkan(int width, int height);
    void cleanup();
    void drawFrame();
    void recordClearCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, const TriangleRenderer::PushConstants& pushConstants);
    void recreateSwapchain();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
private:
    Window window;
    Instance instance;
    Surface surface;
    Device device;
    SwapchainBundle swapchainBundle;
    TriangleRenderer triangle;
    CommandPool commandPool;
    MeshBuffer meshBuffer;
    FrameSync sync;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastFrameTime;
    std::uint32_t frameIndex = 0;
    bool framebufferResized = false;
};
