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
    ~MeshBufffer();

    MeshBuffer(const MeshBuffer&) = delete;
    MeshBuffer& operator=(const MeshBuffer&) = delete;
    MeshBuffer(MeshBuffer&&) noexcept;
    MeshBuffer& operator=(MeshBuffer&&) noexcept;
    VkBuffer vertexBuffer() const { return vbo.get(); }
    uint32_t vertexCount() const { return count; }
private:
    void destroy() noexcept;
    Buffer vbo;
    uint32_t count = 0;
};
