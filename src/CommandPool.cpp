#include "CommandPool.h"
#include <stdexcept>
#include <utility>

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

void CommandPool::destroy() noexcept {
    if (pool) {
        vkDestroyCommandPool(device, pool, nullptr);
    }
    pool = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
    commandBuffers.clear();
}

CommandPool::CommandPool(CommandPool&& o) noexcept 
    : device(o.device), pool(o.pool), commandBuffers(std::move(o.commandBuffers)) {
    o.device = VK_NULL_HANDLE;
    o.pool = VK_NULL_HANDLE;
}

CommandPool& CommandPool::operator=(CommandPool&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    pool = o.pool;
    commandBuffers = std::move(o.commandBuffers);
    o.device = VK_NULL_HANDLE;
    o.pool = VK_NULL_HANDLE;
    return *this;
}

CommandPool::~CommandPool (){
    destroy();
}

const std::vector<VkCommandBuffer>& CommandPool::buffers() const {
    return commandBuffers;
}

std::vector<VkCommandBuffer> CommandPool::allocate(uint32_t count) {
    if (count == 0) {
        throw std::invalid_argument("CommandPool::allocate: count must be greater than zero");
    }
    std::vector<VkCommandBuffer> buffers(count);
    VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.commandPool = pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = count;
    if (vkAllocateCommandBuffers(device, &ai, buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("CommandPool::allocate: vkAllocateCommandBuffers failed"); 
    }
    return buffers;
}
