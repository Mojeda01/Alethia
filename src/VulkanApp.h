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
#include "UniformBuffer.h"
#include "Camera.h"
#include "TextureImage.h"
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
    void cleanup();
    void drawFrame();
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, const TriangleRenderer::PushConstants& pushConstants);
    void recreateSwapchain();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
private:
    Window window;
    Instance instance;
    Surface surface;
    Device device;
    SwapchainBundle swapchainBundle; 
    UniformBuffer uniformBuffer;
    TriangleRenderer triangle;
    CommandPool commandPool;
    MeshBuffer meshBuffer;
    TextureImage texture;
    FrameSync sync;
    Camera camera;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastFrameTime;
    std::uint32_t frameIndex = 0;
    bool framebufferResized = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    bool firstMouse = true;
};
