#include "MeshBuffer.h"

#include <cstring>
#include <stdexcept>
#include <utility>

void MeshBuffer::uploadBuffer(VkDevice device,
                              VkPhysicalDevice physicalDevice,
                              VkCommandPool commandPool,
                              VkQueue transferQueue,
                              Buffer& dst,
                              const void* data,
                              VkDeviceSize byteSize) {
    Buffer staging(device,
                   physicalDevice,
                   byteSize,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* mapped = staging.map();
    std::memcpy(mapped, data, static_cast<size_t>(byteSize));
    staging.unmap();

    VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.commandPool = commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &ai, &cmd) != VK_SUCCESS) {
        throw std::runtime_error("MeshBuffer: vkAllocateCommandBuffers failed");
    }

    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) {
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
        throw std::runtime_error("MeshBuffer: vkBeginCommandBuffer failed");
    }

    VkBufferCopy region{};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = byteSize;
    vkCmdCopyBuffer(cmd, staging.get(), dst.get(), 1, &region);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
        throw std::runtime_error("MeshBuffer: vkEndCommandBuffer failed");
    }

    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;

    if (vkQueueSubmit(transferQueue, 1, &si, VK_NULL_HANDLE) != VK_SUCCESS) {
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
        throw std::runtime_error("MeshBuffer: vkQueueSubmit failed");
    }

    vkQueueWaitIdle(transferQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}

MeshBuffer::MeshBuffer(VkDevice device,
                       VkPhysicalDevice physicalDevice,
                       VkCommandPool commandPool,
                       VkQueue transferQueue,
                       const std::vector<Vertex>& vertices)
    : vbo(device,
          physicalDevice,
          static_cast<VkDeviceSize>(sizeof(Vertex) * vertices.size()),
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
      ibo(device, physicalDevice, 4,
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
      count(static_cast<uint32_t>(vertices.size())),
      idxCount(0)
{
    if (vertices.empty()) {
        throw std::invalid_argument("MeshBuffer: vertex list must not be empty");
    }

    VkDeviceSize vboSize = static_cast<VkDeviceSize>(sizeof(Vertex) * vertices.size());
    uploadBuffer(device, physicalDevice, commandPool, transferQueue, vbo,
                 vertices.data(), vboSize);
}

MeshBuffer::MeshBuffer(VkDevice device,
                       VkPhysicalDevice physicalDevice,
                       VkCommandPool commandPool,
                       VkQueue transferQueue,
                       const std::vector<Vertex>& vertices,
                       const std::vector<uint32_t>& indices)
    : vbo(device,
          physicalDevice,
          static_cast<VkDeviceSize>(sizeof(Vertex) * vertices.size()),
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
      ibo(device,
          physicalDevice,
          static_cast<VkDeviceSize>(sizeof(uint32_t) * (indices.empty() ? 1 : indices.size())),
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
      count(static_cast<uint32_t>(vertices.size())),
      idxCount(static_cast<uint32_t>(indices.size()))
{
    if (vertices.empty()) {
        throw std::invalid_argument("MeshBuffer: vertex list must not be empty");
    }
    if (indices.empty()) {
        throw std::invalid_argument("MeshBuffer: index list must not be empty");
    }

    VkDeviceSize vboSize = static_cast<VkDeviceSize>(sizeof(Vertex) * vertices.size());
    uploadBuffer(device, physicalDevice, commandPool, transferQueue, vbo,
                 vertices.data(), vboSize);

    VkDeviceSize iboSize = static_cast<VkDeviceSize>(sizeof(uint32_t) * indices.size());
    uploadBuffer(device, physicalDevice, commandPool, transferQueue, ibo,
                 indices.data(), iboSize);
}

void MeshBuffer::destroy() noexcept {
    count = 0;
    idxCount = 0;
}

MeshBuffer::MeshBuffer(MeshBuffer&& o) noexcept
    : vbo(std::move(o.vbo)),
      ibo(std::move(o.ibo)),
      count(o.count),
      idxCount(o.idxCount)
{
    o.count = 0;
    o.idxCount = 0;
}

MeshBuffer& MeshBuffer::operator=(MeshBuffer&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    vbo = std::move(o.vbo);
    ibo = std::move(o.ibo);
    count = o.count;
    idxCount = o.idxCount;
    o.count = 0;
    o.idxCount = 0;
    return *this;
}

MeshBuffer::~MeshBuffer() {
    destroy();
}
