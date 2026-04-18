#include "initv2.h"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>
#include <cstdlib>

#include "vkInstance.h"
#include "VkPhysicalDevice.h"
#include "VkLogicalDevice.h"
#include "VkSwapchain.h"
#include "VkPipeline.h"
#include "VkRenderer.h"

static constexpr int kWidth  = 1280;
static constexpr int kHeight = 720;

void runAlethiaV2()
{
    
    setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "0", 1);

    // GLFW
    if (!glfwInit()) {
        throw std::runtime_error("glfwInit failed");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Alethia v2", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    // Core Vulkan init 
    VkInstance instance = createInstance();
    VkDebugUtilsMessengerEXT messenger = createDebugMessenger(instance);
    VkSurfaceKHR surface = createSurface(instance, window);
    VkPhysicalDevice physical = selectPhysicalDevice(instance);

    QueueFamilyIndices queueFamilies = findQueueFamilies(physical, surface);

    VkPhysicalDeviceFeatures features10{};
    VkPhysicalDeviceVulkan12Features features12{};
    VkPhysicalDeviceVulkan13Features features13{};
    buildEnabledFeatures(physical, features10, features12, features13);

    LogicalDevice logical = createLogicalDevice(physical, queueFamilies, features10, features12, features13);
    Renderer renderer = createRenderer(physical, logical, surface, queueFamilies, window);

    // Framebuffer resize callback
    glfwSetWindowUserPointer(window, &renderer);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int, int) {
            auto* r = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
            notifyFramebufferResized(*r);
    });
    
    // Shader hot-reload on R key 
    glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int, int action, int) {
        if (key == GLFW_KEY_R && action == GLFW_PRESS) {
            auto* r = static_cast<Renderer*>(glfwGetWindowUserPointer(w));
            std::cout << "[Vulkan] Reloading shaders (R pressed)...\n";

            const bool success = reloadTrianglePipeline(
                r->logicalDevice.device,
                r->pipeline,
                r->swapchain.imageFormat,
                r->depth.format,
                ALETHIA_V2_SHADER_DIR "/v2_triangle.vert.spv",
                ALETHIA_V2_SHADER_DIR "/v2_triangle.frag.spv"
            );

            if (success) {
                std::cout << "[Vulkan] Pipeline reloaded — animation should now be active!\n";
            }
        }
    });

    // Pipeline
    renderer.pipeline = createTrianglePipeline(
        logical.device,
        renderer.swapchain.imageFormat,
        renderer.depth.format,
        ALETHIA_V2_SHADER_DIR "/v2_triangle.vert.spv",
        ALETHIA_V2_SHADER_DIR "/v2_triangle.frag.spv"
    );

    // Game loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame(renderer);
    }

    // Cleanup
    destroyPipeline(logical.device, renderer.pipeline);
    destroyRenderer(renderer);
    destroySurface(instance, surface);
    destroyDebugMessenger(instance, messenger);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}
