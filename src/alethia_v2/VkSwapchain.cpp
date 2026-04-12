#include "VkSwapchain.h"
#include "VkPhysicalDevice.h"

#include <stdexcept>
#include <algorithm>
#include <limits>
#include <iostream>


static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available)
{
    // Prefer SRGB 32-bit with nonlinear color space.
    for (const auto& f : available) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return f;
        }
    }
    
    // Fall back to the first available format.
    return available[0];
}

static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& available)
{
    // Prefer mailbox
    for (const auto& mode : available) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }
    // FIFO is guaranteed to be present 
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window)
{
    // If the driver has set a fixed extent, use it 
    if (caps.currentextent.width != std::numeric_limits<uint32_t>::max())
        return caps.currentExtent;

    // Otherwise clamp the framebuffer size to the allowed range.
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);

    VkExtent2D extent {
        static_cast<uint32_t>(w),
        static_cast<uint32_t>(h)
    };
    extent.width = std::clamp(extent.width,  caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return extent;
}

static std::vector<VkImageView> createImageViews(
    VkDevice device,
    const std::vector<VkImage>& images,
    VkFormat format
){
    std::vector<VkImageView> views;
    views.reserve(images.size());

    for (VkImage image : images) {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = format;

        info.components.r = VK_COMPONENTS_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENTS_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENTS_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENTS_SWIZZLE_IDENTITY;

        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;

        VkImageView view = VK_NULL_HANDLE;
        if (vkCreateImageView(device, &info, nullptr, &view) != VK_SUCCESS)
            throw std::runtime_error("Failed to create swapchain image view");

        view.push_back(view);
    }
    return views;
}

// Public API
VkSurfaceKHR createSurface(vkInstance instance, GLFWwindow* window)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("glfwCreateWindowSurface failed");

    return surface;
}

void destroySurface(VkInstance instance, VkSurfaceKHR surface)
{
    if (!surface != VK_NULL_HANDLE){
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
}

Swapchain createSwapchain(
    VkPhysicalDevice          physical,
    const LogicalDevice&      ld,
    VkSurfaceKHR              surface,
    const QueueFamilyIndices& indices,
    GLFWwindow*               window,
    VkSwapchainKHR            oldSwapchain)
{
    const SwapChainSupportDetails support = querySwapChainSupport(physical, surface);
    if (support.formats.empty() || support.presentModes.empty()){
        throw std::runtime_error("Physical device has no swapchain formats or present modes");
    }
        
    const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(support.formats);
    const VkPresentModeKHR presentMode = choosePresentMode(support.presentModes);
    const VkExtent2D extent = chooseExtent(support.capabilities, window);

    // Request one more image than the minimum to avoid stalling on the driver.
    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0){
        imageCount = std::min(imageCount, support.capabilities.maxImageCount);
    }
        
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform     = support.capabilities.currentTransform;
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode      = presentMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.oldSwapchain     = oldSwapchain;

    // If graphics and present are on different queue families, share the images
    const uint32_t queueFamilies[] = { *indices.graphics, *indices.present };
    if (*indices.graphics != *indices.present) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilies;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    Swapchain sc;
    if (vkCreateSwapchainKHR(ld.device, &createInfo, nullptr, &sc.handle) !=  VK_SUCCESS){
        throw std::runtime_error("vkCreateSwapchainKHR failed");
    }

    // Retrieve image handles
    uint32_t actualCount = 0;
    vkGetSwapchainImagesKHR(ld.device, sc.handle, &actualCount, nullptr);
    sc.images.resize(actualCount);
    vkGetSwapchainImagesKHR(ld.device, sc.handle, &actualCount,  sc.images.data());

    sc.imageFormat = surfaceFormat.format;
    sc.colorSpace  = surfaceFormat.colorSpace;
    sc.extent      = extent;
    sc.imageViews  = createImageViews(ld.device, sc.images, sc.imageFormat);

    std::cout << "[Vulkan] Swapchain created — "
        << actualCount << " images, "
        << extent.width << "x" << extent.height
        << ", present mode: "
        << (presentMode == VK_PRESENT_MODE_MAILBOX_KHR ? "Mailbox" : "FIFO")
        << "\n";

    return sc;
}

void destroySwapchain(VkDevice device, Swapchain& sc)
{
    for (VkImageView view : sc.imageViews){
        vkDestroyImageView(device, view, nullptr);
    }
    sc.imageViews.clear();

    if (sc.handle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, sc.handle, nullptr);
        sc.handle = VK_NULL_HANDLE;
    }

    sc.images.clear();
    sc.extent       = {};
    sc.imageFormat  = VK_FORMAT_UNDEFINED;
    sc.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
}
