#pragma once

#include <vulkan/vulkan.h>
#include <cstddef>

class Buffer{
public:
    Buffer( VkDevice device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags memoryProperties);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept;
    Buffer& operator=(Buffer&&) noexcept;

    VkBuffer get() const { return buffer; }
    VkDeviceMemory memory() const { return mem; }
    VkDeviceSize size() const { return bufferSize; }

    void* map();
    void unmap();

private:
    void destroy() noexcept;
    static uint32_t findMemoryType( VkPhysicalDevice physicalDevice,
                                    uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties);
    VkDevice device = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkDeviceSize bufferSize = 0;
    void* mapped = nullptr;
};
