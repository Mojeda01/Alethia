#pragma once 
#include <vulkan/vulkan.h>
#include <vector>

class CommandPool{
public:
    CommandPool(VkDevice device, uint32_t queueFamilyIndex, size_t bufferCount);
    ~CommandPool();

    const std::vector<VkCommandBuffer>& buffers() const;
    std::vector<VkCommandBuffer> allocate(uint32_t count); // allocating command buffers.
private:
    VkDevice device;
    VkCommandPool pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
};
