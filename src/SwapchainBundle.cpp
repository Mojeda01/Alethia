#include "SwapchainBundle.h"
#include <utility>

SwapchainBundle::SwapchainBundle(   VkPhysicalDevice phys,
                                    VkDevice device,
                                    VkSurfaceKHR surface,
                                    uint32_t width,
                                    uint32_t height)
        :   swapchainObj(phys, device, surface, width, height)
        , renderPassObj(device, swapchainObj.imageFormat())
        , framebufferSet(   device,
                            renderPassObj.get(),
                            swapchainObj.extent(),
                            swapchainObj.imageViews())
{
}

void SwapchainBundle::recreate(VkPhysicalDevice phys,
                                VkDevice device,
                                VkSurfaceKHR surface,
                                uint32_t width,
                                uint32_t height)
{
    // rely on RAII destructors of members
    *this = SwapchainBundle(phys, device, surface, width, height);
    SwapchainBundle tmp(phys, device, surface, width, height);
    *this = std::move(tmp);
}

VkRenderPass SwapchainBundle::renderPass() const {
    return renderPassObj.get();
}

VkExtent2D SwapchainBundle::extent() const {
    return swapchainObj.extent();
}

VkSwapchainKHR SwapchainBundle::swapchain() const {
    return swapchainObj.get();
}

const std::vector<VkFramebuffer>& SwapchainBundle::framebuffers() const {
    return framebufferSet.get();
}
