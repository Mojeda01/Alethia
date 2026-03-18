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
#include "ImGuiLayer.h"
#include "DebugUI.h"
#include "GridRenderer.h"
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
    GridRenderer grid;
    CommandPool commandPool;
    MeshBuffer gridMesh;
    FrameSync sync;
    Camera camera;
    ImGuiLayer imgui;
    DebugUI debugUI;
    bool uiMode = false;
    float lightPos[3] = { 5.0f, 10.0f, 5.0f }; 
    bool wireframe = false;
    static constexpr int FRAME_TIME_COUNT = 120;
    float frameTimes[FRAME_TIME_COUNT] = {};
    int frameTimeIndex = 0;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastFrameTime;
    std::uint32_t frameIndex = 0;
    bool framebufferResized = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    bool firstMouse = true;
};
