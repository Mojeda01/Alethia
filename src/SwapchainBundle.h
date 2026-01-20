#pragma once
#include "Swapchain.h"
#include "RenderPass.h"
#include "Framebuffer.h"

class SwapchainBundle{
public:
    SwapchainBundle(VkPhysicalDevice phys,
                    VkDevice device,
                    VkSurfaceKHR surface,
                    uint32_t width,
                    uint32_t height);
    void recreate(VkPhysicalDevice phys,
                  VkDevice device,
                  VkSurfaceKHR surface,
                  uint32_t width,
                  uint32_t height);

    VkRenderPass renderPass() const;
    VkExtent2D extent() const;
    VkSwapchainKHR swapchain() const;
    const std::vector<VkFramebuffer>& framebuffers() const;
private:
    void create(VkPhysicalDevice phys,
                VkDevice device,
                VkSurfaceKHR surface,
                uint32_t width,
                uint32_t height);
private:
    Swapchain swapchainObj;
    RenderPass renderPassObj;
    FramebufferSet framebufferSet;
};
