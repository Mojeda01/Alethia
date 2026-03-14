#pragma once
#include "Swapchain.h"
#include "DepthImage.h"
#include "RenderPass.h"
#include "Framebuffer.h"

class SwapchainBundle{
public:
    SwapchainBundle(VkPhysicalDevice phys,
                    VkDevice device,
                    VkSurfaceKHR surface,
                    uint32_t width,
                    uint32_t height);
    SwapchainBundle(const SwapchainBundle&) = delete;
    SwapchainBundle& operator=(const SwapchainBundle&) = delete;
    SwapchainBundle(SwapchainBundle&&) noexcept = default;
    SwapchainBundle& operator=(SwapchainBundle&&) noexcept = default;
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
    DepthImage depthImage;
    RenderPass renderPassObj;
    FramebufferSet framebufferSet;
};
