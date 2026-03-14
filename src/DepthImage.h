#pragma once

#include <vulkan/vulkan.h>

class DepthImage{
public:
    DepthImage( VkDevice device,
                VkPhysicalDevice physicalDevice,
                VkExtent2D extent);
    ~DepthImage();

    DepthImage(const DepthImage&) = delete;
    DepthImage& operator=(const DepthImage&) = delete;
    DepthImage(DepthImage&&) noexcept;
    DepthImage& operator=(DepthImage&&) noexcept;

    VkImageView view() const { return imageView; }
    VkFormat format() const { return depthFormat; }
    static VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice);

private:
    void destroy() noexcept;
    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                                    uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties);
    VkDevice device = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
};
