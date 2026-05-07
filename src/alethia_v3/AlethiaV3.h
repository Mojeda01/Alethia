#pragma once 
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

class AlethiaV3{
public:
    explicit AlethiaV3(GLFWwindow* window);
    ~AlethiaV3();

    void drawFrame();
    void initVulkan();
    bool isInitialized() const { return m_initialized; }
private:
    GLFWwindow* m_window = nullptr;

    // Vulkan objects
    VkInstance                  m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice            m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                    m_device = VK_NULL_HANDLE;
    VkQueue                     m_graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR                m_surface = VK_NULL_HANDLE;

    VkSwapchainKHR              m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage>        m_swapchainImages;
    std::vector<VkImageView>    m_swapchainImageViews;
    VkFormat                    m_swapchainImageFormat{};
    VkExtent2D                  m_swapchainExtent{};

    VkRenderPass                m_renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer>  m_framebuffers;

    VkCommandPool               m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;

    // sync
    std::vector<VkSemaphore>    m_imageAvailableSemaphores;
    std::vector<VkSemaphore>    m_renderFinishedSemaphores;
    std::vector<VkFence>        m_inFlightFences;
    uint32_t                    m_currentFrame = 0;

    bool                        m_initialized = false;
 
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void recreateSwapchain();

    void cleanupSwapchain();
    void cleanup();
};
