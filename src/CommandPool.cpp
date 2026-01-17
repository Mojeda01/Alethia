#include "CommandPool.h"
#include <stdexcept>

CommandPool::CommandPool(VkDevice dev, uint32_t queueFamilyIndex, size_t bufferCount) : device(dev)
{
    VkCommandPoolCreateInfo pci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pci.queueFamilyIndex = queueFamilyIndex;
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &pci, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("Command pool creation failed");
    }
    commandBuffers.resize(bufferCount);

    VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &ai, commandBuffers.data()) != VK_SUCCESS){
        throw std::runtime_error("Command buffer allocation failed");
    }
}

CommandPool::~CommandPool() {
    if (pool) {
        vkDestroyCommandPool(device, pool, nullptr);
    }
}

const std::vector<VkCommandBuffer>& CommandPool::buffers() const {
    return commandBuffers;
}
