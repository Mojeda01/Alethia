#pragma once

#include "Buffer.h"
#include "Vertex.h"

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

class MeshBuffer{
public:

    MeshBuffer( VkDevice device,
                VkPhysicalDevice physicalDevice,
                VkCommandPool commandPool,
                VkQueue transferQueue,
                const std::vector<Vertex>& vertices);

    MeshBuffer( VkDevice device,
                VkPhysicalDevice physicalDevice,
                VkCommandPool commandPool,
                VkQueue transferQueue,
                const std::vector<Vertex>& vertices,
                const std::vector<uint32_t>& indices);

    ~MeshBuffer();

    MeshBuffer(const MeshBuffer&) = delete;
    MeshBuffer& operator=(const MeshBuffer&) = delete;
    MeshBuffer(MeshBuffer&&) noexcept;
    MeshBuffer& operator=(MeshBuffer&&) noexcept;
    VkBuffer vertexBuffer() const { return vbo.get(); }
    uint32_t vertexCount() const { return count; }
    VkBuffer indexBuffer() const { return ibo.get(); }
    uint32_t indexCount() const { return idxCount; }
    bool hasIndices() const { return idxCount > 0; }
private:
    void destroy() noexcept;
    void uploadBuffer(  VkDevice device,
                        VkPhysicalDevice physicalDevice,
                        VkCommandPool commandPool,
                        VkQueue transferQueue,
                        Buffer& dst,
                        const void* data,
                        VkDeviceSize byteSize);
    Buffer vbo;
    Buffer ibo;
    uint32_t count = 0;
    uint32_t idxCount = 0;
};
