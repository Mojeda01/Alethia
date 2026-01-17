#include "Swapchain.h"
#include <stdexcept>
#include <algorithm>

Swapchain::Swapchain(VkPhysicalDevice phys,
        VkDevice dev,
        VkSurfaceKHR surface,
        uint32_t width,
        uint32_t height) 
    : device(dev) {

   VkSurfaceCapabilitiesKHR caps;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);

   uint32_t formatCount;
   vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &formatCount, nullptr);
   std::vector<VkSurfaceFormatKHR> formats(formatCount);
   vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &formatCount, formats.data());

   uint32_t modeCount;
   vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &modeCount, nullptr);
   std::vector<VkPresentModeKHR> modes(modeCount);
   vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &modeCount, modes.data());

   auto surfaceFormat = chooseFormat(formats);
   auto presentMode = choosePresentMode(modes);
   auto extent = chooseExtent(caps, width, height);

   uint32_t imageCount = caps.minImageCount + 1;
   if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
   }

    VkSwapchainCreateInfoKHR ci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    ci.surface = surface;
    ci.minImageCount = imageCount;
    ci.imageFormat = surfaceFormat.format;
    ci.imageColorSpace = surfaceFormat.colorSpace;
    ci.imageExtent = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform = caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = presentMode;
    ci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &ci, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Swapchain creation failed");
    }

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

    format = surfaceFormat.format;
    swapExtent = extent;

    views.resize(images.size());
    for (size_t i = 0; i < images.size(); ++i){
        VkImageViewCreateInfo iv{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        iv.image = images[i];
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = format;
        iv.components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount = 1;
        iv.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &iv, nullptr, &views[i]) != VK_SUCCESS) {
            throw std::runtime_error("Image view creation failed");
        }
    }
}

Swapchain::~Swapchain() {
    for (auto v : views) {
        vkDestroyImageView(device, v, nullptr);
    }
    if (swapchain) {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }
}

VkSwapchainKHR Swapchain::get() const { return swapchain; }
VkFormat Swapchain::imageFormat() const { return format; }
VkExtent2D Swapchain::extent() const { return swapExtent; }
const std::vector<VkImageView>& Swapchain::imageViews() const { return views; }

VkSurfaceFormatKHR Swapchain::chooseFormat(const std::vector<VkSurfaceFormatKHR>& format) {
    for (const auto& f : format) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && 
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }
    return format[0];
}

VkPresentModeKHR Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (auto m : modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
            return m;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& caps,
                                    uint32_t width, uint32_t height) {
    if (caps.currentExtent.width != UINT32_MAX) {
        return caps.currentExtent;
    }

    VkExtent2D e;
    e.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
    e.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return e;
}
