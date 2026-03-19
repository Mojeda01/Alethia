#pragma once

#include <vulkan/vulkan.h>
#include <string>

class TextureImage{
public:
    TextureImage(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue transferQueue,
        const std::string& filePath
    );

    TextureImage(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue transferQueue,
        const unsigned char* pixels,
        int width, int height
    );

    TextureImage(
        VkDevice device,
        VkPhysicalDevice physicalDevice
    );

    ~TextureImage();

    TextureImage(const TextureImage&) = delete;
    TextureImage& operator=(const TextureImage&) = delete;
    TextureImage(TextureImage&&) noexcept;
    TextureImage& operator=(TextureImage&&) noexcept; 

    VkSampler sampler() const { return texSampler; }
    VkImageView view() const { return imageView; }

private: 
    void createFromPixels(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue transferQueue,
        const unsigned char* pixels,
        int width, int height
    );
    void createSampler();
    void destroy() noexcept;
    static uint32_t findMemoryType(
        VkPhysicalDevice physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties
    );
    VkDevice device = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler texSampler = VK_NULL_HANDLE;
};
