#include "UniformBuffer.h"

#include <cstring>
#include <stdexcept>
#include <utility>

uint32_t UniformBuffer::findMemoryType(VkPhysicalDevice physicalDevice,
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

    throw std::runtime_error("UniformBuffer: failed to find suitable memory type");
}

UniformBuffer::UniformBuffer(VkDevice dev,
                             VkPhysicalDevice physicalDevice,
                             uint32_t count)
    : device(dev), frameCount(count)
{
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
        throw std::invalid_argument("UniformBuffer: invalid Vulkan handles");
    }
    if (frameCount == 0) {
        throw std::invalid_argument("UniformBuffer: frameCount must be greater than zero");
    }

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; 

    VkDescriptorSetLayoutCreateInfo layoutCi{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutCi.bindingCount = 1;
    layoutCi.pBindings = &binding;

    if (vkCreateDescriptorSetLayout(device, &layoutCi, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("UniformBuffer: vkCreateDescriptorSetLayout failed");
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = frameCount;

    VkDescriptorPoolCreateInfo poolCi{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolCi.maxSets = frameCount;
    poolCi.poolSizeCount = 1;
    poolCi.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(device, &poolCi, nullptr, &pool) != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
        layout = VK_NULL_HANDLE;
        throw std::runtime_error("UniformBuffer: vkCreateDescriptorPool failed");
    }

    buffers.resize(frameCount, VK_NULL_HANDLE);
    memories.resize(frameCount, VK_NULL_HANDLE);
    mappedPtrs.resize(frameCount, nullptr);

    VkDeviceSize bufferSize = sizeof(MVPData);

    for (uint32_t i = 0; i < frameCount; ++i) {
        VkBufferCreateInfo bci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bci.size = bufferSize;
        bci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bci, nullptr, &buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("UniformBuffer: vkCreateBuffer failed");
        }

        VkMemoryRequirements memReqs{};
        vkGetBufferMemoryRequirements(device, buffers[i], &memReqs);

        VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        ai.allocationSize = memReqs.size;
        ai.memoryTypeIndex = findMemoryType(physicalDevice,
                                            memReqs.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &ai, nullptr, &memories[i]) != VK_SUCCESS) {
            throw std::runtime_error("UniformBuffer: vkAllocateMemory failed");
        }

        if (vkBindBufferMemory(device, buffers[i], memories[i], 0) != VK_SUCCESS) {
            throw std::runtime_error("UniformBuffer: vkBindBufferMemory failed");
        }

        if (vkMapMemory(device, memories[i], 0, bufferSize, 0, &mappedPtrs[i]) != VK_SUCCESS) {
            throw std::runtime_error("UniformBuffer: vkMapMemory failed");
        }
    }

    std::vector<VkDescriptorSetLayout> layouts(frameCount, layout);

    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = frameCount;
    allocInfo.pSetLayouts = layouts.data();

    sets.resize(frameCount);
    if (vkAllocateDescriptorSets(device, &allocInfo, sets.data()) != VK_SUCCESS) {
        throw std::runtime_error("UniformBuffer: vkAllocateDescriptorSets failed");
    }

    for (uint32_t i = 0; i < frameCount; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = bufferSize;

        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = sets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

void UniformBuffer::destroy() noexcept {
    if (device != VK_NULL_HANDLE) {
        for (uint32_t i = 0; i < frameCount; ++i) {
            if (mappedPtrs[i] != nullptr && memories[i] != VK_NULL_HANDLE) {
                vkUnmapMemory(device, memories[i]);
            }
            if (buffers[i] != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, buffers[i], nullptr);
            }
            if (memories[i] != VK_NULL_HANDLE) {
                vkFreeMemory(device, memories[i], nullptr);
            }
        }
        if (pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, pool, nullptr);
        }
        if (layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, layout, nullptr);
        }
    }
    buffers.clear();
    memories.clear();
    mappedPtrs.clear();
    sets.clear();
    pool = VK_NULL_HANDLE;
    layout = VK_NULL_HANDLE;
    frameCount = 0;
    device = VK_NULL_HANDLE;
}

UniformBuffer::UniformBuffer(UniformBuffer&& o) noexcept
    : device(o.device),
      layout(o.layout),
      pool(o.pool),
      sets(std::move(o.sets)),
      buffers(std::move(o.buffers)),
      memories(std::move(o.memories)),
      mappedPtrs(std::move(o.mappedPtrs)),
      frameCount(o.frameCount)
{
    o.device = VK_NULL_HANDLE;
    o.layout = VK_NULL_HANDLE;
    o.pool = VK_NULL_HANDLE;
    o.frameCount = 0;
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    layout = o.layout;
    pool = o.pool;
    sets = std::move(o.sets);
    buffers = std::move(o.buffers);
    memories = std::move(o.memories);
    mappedPtrs = std::move(o.mappedPtrs);
    frameCount = o.frameCount;
    o.device = VK_NULL_HANDLE;
    o.layout = VK_NULL_HANDLE;
    o.pool = VK_NULL_HANDLE;
    o.frameCount = 0;
    return *this;
}

UniformBuffer::~UniformBuffer() {
    destroy();
}

void UniformBuffer::update(uint32_t frameIndex, const MVPData& data) {
    std::memcpy(mappedPtrs[frameIndex], &data, sizeof(MVPData));
}
