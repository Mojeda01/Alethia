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
    void recordClearCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    void recreateSwapchain();
private:
    Window window;
    Instance instance;
    Surface surface;
    Device device;
    SwapchainBundle swapchainBundle;
    CommandPool commandPool;
    FrameSync sync;
};
