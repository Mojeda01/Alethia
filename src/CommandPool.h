#pragma once 
#include <vulkan/vulkan.h>
#include <vector>

class CommandPool{
public:
    CommandPool(VkDevice device, uint32_t queueFamilyIndex, size_t bufferCount);
    ~CommandPool();
    
    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool(CommandPool&&) noexcept;
    CommandPool& operator=(CommandPool&&) noexcept;

    const std::vector<VkCommandBuffer>& buffers() const;
    std::vector<VkCommandBuffer> allocate(uint32_t count); // allocating command buffers.
private:
    void destroy() noexcept;
    VkDevice device;
    VkCommandPool pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
};
