#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;

class ImGuiLayer {
public:
    ImGuiLayer(GLFWwindow* window,
                 VkInstance instance,
                 VkPhysicalDevice physicalDevice,
                 VkDevice device,
                 uint32_t graphicsQueueFamily,
                 VkQueue graphicsQueue,
                 VkRenderPass renderPass,
                 uint32_t imageCount);
    ~ImGuiLayer(); 

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;
    ImGuiLayer(ImGuiLayer&&) = delete;
    ImGuiLayer& operator=(ImGuiLayer&&) = delete;

    void newFrame();
    void render(VkCommandBuffer cmd);

    bool wantCaptureMouse() const;
    bool wantCaptureKeyboard() const;

private:
    VkDevice device = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
};


