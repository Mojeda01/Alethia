#include "DepthImage.h"

#include <stdexcept>
#include <utility>
#include <array>
#include <iostream>

VkFormat DepthImage::findSupportedFormat(VkPhysicalDevice physicalDevice) {
    const std::array<VkFormat, 3> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT 
    };

    for (VkFormat fmt : candidates) {
        VkFormatProperties props{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, fmt, &props);

        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return fmt;
        }
    }
    throw std::runtime_error("DepthImage: no supported depth format found");
}

uint32_t DepthImage::findMemoryType(    VkPhysicalDevice physicalDevice,
                                        uint32_t typeFilter,
                                        VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) &&
                (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("DepthImage: failed to find suitable memory type");
}

DepthImage::DepthImage( VkDevice dev,
                        VkPhysicalDevice physicalDevice,
                        VkExtent2D extent) : device(dev) 
{
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
        throw std::invalid_argument("DepthImage: invalid Vulkan handles");
    }
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("DepthImage: extent must be non-zero");
    }
    
    depthFormat = findSupportedFormat(physicalDevice);
    std::cout << "Depth format: " << depthFormat << "\n";

    VkImageCreateInfo ici{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.format = depthFormat;
    ici.extent.width = extent.width;
    ici.extent.height = extent.height;
    ici.extent.depth = 1;
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &ici, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("DepthImage: vkCreateImage failed");
    }
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device, image, &memReqs);

    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = memReqs.size;
    ai.memoryTypeIndex = findMemoryType(physicalDevice,
                                        memReqs.memoryTypeBits,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
        throw std::runtime_error("DepthImage: vkAllocateMemory failed");
    }

    if (vkBindImageMemory(device, image, mem, 0) != VK_SUCCESS) {
        vkFreeMemory(device, mem, nullptr);
        vkDestroyImage(device, image, nullptr);
        mem = VK_NULL_HANDLE;
        image = VK_NULL_HANDLE;
        throw std::runtime_error("DepthImage: vkBindImageMemory failed");
    }

    VkImageViewCreateInfo vci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    vci.image = image;
    vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vci.format = depthFormat;
    vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    vci.subresourceRange.baseMipLevel = 0;
    vci.subresourceRange.levelCount = 1;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &vci, nullptr, &imageView) != VK_SUCCESS) {
        vkFreeMemory(device, mem, nullptr);
        vkDestroyImage(device, image, nullptr);
        mem = VK_NULL_HANDLE;
        image = VK_NULL_HANDLE;
        throw std::runtime_error("DepthImage: vkCreateImageView failed");
    }
}

void DepthImage::destroy() noexcept {
    if (device != VK_NULL_HANDLE) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(device, image, nullptr);
        }
        if (mem != VK_NULL_HANDLE) {
            vkFreeMemory(device, mem, nullptr);
        }
    }
    imageView = VK_NULL_HANDLE;
    image = VK_NULL_HANDLE;
    mem = VK_NULL_HANDLE;
    depthFormat = VK_FORMAT_UNDEFINED;
    device = VK_NULL_HANDLE;
}

DepthImage::DepthImage(DepthImage&& o) noexcept
    : device(o.device),
      image(o.image),
      mem(o.mem),
      imageView(o.imageView),
      depthFormat(o.depthFormat)
{
    o.device = VK_NULL_HANDLE;
    o.image = VK_NULL_HANDLE;
    o.mem = VK_NULL_HANDLE;
    o.imageView = VK_NULL_HANDLE;
    o.depthFormat = VK_FORMAT_UNDEFINED;
}

DepthImage& DepthImage::operator=(DepthImage&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    image = o.image;
    mem = o.mem;
    imageView = o.imageView;
    depthFormat = o.depthFormat;
    o.device = VK_NULL_HANDLE;
    o.image = VK_NULL_HANDLE;
    o.mem = VK_NULL_HANDLE;
    o.imageView = VK_NULL_HANDLE;
    o.depthFormat = VK_FORMAT_UNDEFINED;
    return *this;
}

DepthImage::~DepthImage() {
    destroy();
}
