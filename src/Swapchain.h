#pragma once 
#include <vulkan/vulkan.h>
#include <vector>

class Swapchain {
public:
    Swapchain(VkPhysicalDevice phys,
            VkDevice device,
            VkSurfaceKHR surface,
            uint32_t width,
            uint32_t height);

    ~Swapchain();

    VkSwapchainKHR get() const;
    VkFormat imageFormat() const;
    VkExtent2D extent() const;
    const std::vector<VkImageView>& imageViews() const;

private:
    VkDevice device;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    VkFormat format;
    VkExtent2D swapExtent;

    std::vector<VkImage> images;
    std::vector<VkImageView> views;

    VkSurfaceFormatKHR chooseFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps,
                                uint32_t width, uint32_t height);
};
