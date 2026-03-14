#include "Buffer.h"

#include <stdexcept>
#include <utility>

Buffer::Buffer( VkDevice dev,
                VkPhysicalDevice physicalDevice,
                VkDeviceSize size,
                VkBufferUsageFlags usage,
                VkMemoryPropertyFlags memoryProperties) 
    : device(dev), bufferSize(size)
{
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
        throw std::invalid_argument("Buffer: invalid Vulkan handles");
    }
    if (size == 0) {
        throw std::invalid_argument("Buffer: size must be greater than zero");
    }

    VkBufferCreateInfo bci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bci.size = size;
    bci.usage = usage;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bci, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateBuffer failed");
    }

    VkMemoryRequirements memReqs{};
    vkGetBufferMemoryRequirements(device, buffer, &memReqs);
    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = memReqs.size;
    ai.memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, memoryProperties);

    if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        throw std::runtime_error("vkAllocateMemory failed");
    }

    if (vkBindBufferMemory(device, buffer, mem, 0) != VK_SUCCESS) {
        vkFreeMemory(device, mem, nullptr);
        vkDestroyBuffer(device, buffer, nullptr);
        mem = VK_NULL_HANDLE;
        buffer = VK_NULL_HANDLE;
        throw std::runtime_error("vkBindBufferMemory failed");
    }
}

void Buffer::destroy() noexcept {
    if (device != VK_NULL_HANDLE) {
        if (mapped != nullptr) {
            vkUnmapMemory(device, mem);
            mapped = nullptr;
        }
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffer, nullptr);
        }
        if (mem != VK_NULL_HANDLE) {
            vkFreeMemory(device, mem, nullptr);
        }
    }
    buffer = VK_NULL_HANDLE;
    mem = VK_NULL_HANDLE;
    bufferSize = 0;
    device = VK_NULL_HANDLE;
}

Buffer::Buffer(Buffer&& o) noexcept
    : device(o.device),
      buffer(o.buffer),
      mem(o.mem),
      bufferSize(o.bufferSize),
      mapped(o.mapped)
{
    o.device = VK_NULL_HANDLE;
    o.buffer = VK_NULL_HANDLE;
    o.mem = VK_NULL_HANDLE;
    o.bufferSize = 0;
    o.mapped = nullptr;
}

Buffer& Buffer::operator=(Buffer&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    buffer = o.buffer;
    mem = o.mem;
    bufferSize = o.bufferSize;
    mapped = o.mapped;
    o.device = VK_NULL_HANDLE;
    o.buffer = VK_NULL_HANDLE;
    o.mem = VK_NULL_HANDLE;
    o.bufferSize = 0;
    o.mapped = nullptr;
    return *this;
}

Buffer::~Buffer() {
    destroy();
}

void* Buffer::map() {
    if (mapped != nullptr) {
        return mapped;
    }
    if (vkMapMemory(device, mem, 0, bufferSize, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("vkMapMemory failed");
    }
    return mapped;
}

void Buffer::unmap() {
    if (mapped != nullptr) {
        vkUnmapMemory(device, mem);
        mapped = nullptr;
    }
}

uint32_t Buffer::findMemoryType(VkPhysicalDevice physicalDevice,
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
    throw std::runtime_error("Failed to find suitable memory type");
}
