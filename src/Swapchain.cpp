#include "Swapchain.h"
#include "SwapchainSelection.h"

#include <stdexcept>
#include <algorithm>
#include <utility>

Swapchain::Swapchain(VkPhysicalDevice phys,
        VkDevice dev,
        VkSurfaceKHR surface,
        uint32_t width,
        uint32_t height) 
    : device(dev) {
    if (phys == VK_NULL_HANDLE || dev == VK_NULL_HANDLE || surface == VK_NULL_HANDLE) {
        throw std::invalid_argument("Swapchain: invalid Vulkan handles");
    }

    VkSurfaceCapabilitiesKHR caps;
    VkResult r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);
    if (r != VK_SUCCESS) {
        throw std::runtime_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
    }

    uint32_t formatCount = 0;
    r = vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &formatCount, nullptr);
    if (r != VK_SUCCESS || formatCount == 0) {
        throw std::runtime_error("No surface formats available");
    }
    std::vector<VkSurfaceFormatKHR> formats(formatCount); 
    r = vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &formatCount, formats.data());
    if (r != VK_SUCCESS) {
        throw std::runtime_error("vkGetPhysicalDeviceSurfaceFormatsKHR failed");
    }

    uint32_t modeCount = 0;
    r = vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &modeCount, nullptr);
    if (r != VK_SUCCESS || modeCount == 0) {
        throw std::runtime_error("No present modes available");
    }

    std::vector<VkPresentModeKHR> modes(modeCount);
    r = vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &modeCount, modes.data());
    if (r != VK_SUCCESS) {
        throw std::runtime_error("vkGetPhysicalDeviceSurfacePresentModesKHR failed");
    }

    const VkSurfaceFormatKHR surfaceFormat = swapchain_select::chooseSurfaceFormat(formats);
    const VkPresentModeKHR presentMode = swapchain_select::choosePresentMode(modes);
    const VkExtent2D extent = swapchain_select::chooseExtent(caps, width, height);

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

    r = vkCreateSwapchainKHR(device, &ci, nullptr, &swapchain);
    if (r != VK_SUCCESS) {
        throw std::runtime_error("Swapchain creation failed");
    }

    r = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    if (r != VK_SUCCESS || imageCount == 0) {
        throw std::runtime_error("vkGetSwapchainImagesKHR (count) failed");
    }

    images.resize(imageCount);
    r = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());
    if (r != VK_SUCCESS) {
        throw std::runtime_error("vkGetSwapchainImagesKHR failed");
    }

    format = surfaceFormat.format;
    swapExtent = extent;
    views.resize(images.size(), VK_NULL_HANDLE);

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
    destroy();
}

void Swapchain::destroy() noexcept {
    if (device != VK_NULL_HANDLE) {
        for (auto v : views) {
            if (v) vkDestroyImageView(device, v, nullptr);
        }
        if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
    }
    views.clear();
    images.clear();
    swapchain = VK_NULL_HANDLE;
}

Swapchain::Swapchain(Swapchain&& o) noexcept 
    :   device(o.device),
        swapchain(o.swapchain),
        format(o.format),
        swapExtent(o.swapExtent),
        images(std::move(o.images)),
        views(std::move(o.views)) {
    o.device = VK_NULL_HANDLE;
    o.swapchain = VK_NULL_HANDLE;
}

Swapchain& Swapchain::operator=(Swapchain&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    swapchain = o.swapchain;
    format = o.format;
    swapExtent = o.swapExtent;
    images = std::move(o.images);
    views = std::move(o.views);
    o.device = VK_NULL_HANDLE;
    o.swapchain = VK_NULL_HANDLE;
    return *this;
}

VkSwapchainKHR Swapchain::get() const { return swapchain; }
VkFormat Swapchain::imageFormat() const { return format; }
VkExtent2D Swapchain::extent() const { return swapExtent; }
const std::vector<VkImageView>& Swapchain::imageViews() const { return views; }


