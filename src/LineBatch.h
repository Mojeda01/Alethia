#pragma once

#include "LineVertex.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

class LineBatch {
public:
    LineBatch(VkDevice device, VkPhysicalDevice physicalDevice,
              VkCommandPool commandPool, VkQueue transferQueue,
              uint32_t maxVertices = 4096);
    ~LineBatch();

    LineBatch(const LineBatch&) = delete;
    LineBatch& operator=(const LineBatch&) = delete;
    LineBatch(LineBatch&&) noexcept;
    LineBatch& operator=(LineBatch&&) noexcept;

    void clear();
    void addLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& color);
    void addAABBEdges(const glm::vec3& min, const glm::vec3& max,
                      const glm::vec3& colorX, const glm::vec3& colorY, const glm::vec3& colorZ);
    void upload();

    VkBuffer buffer() const { return vkBuffer; }
    uint32_t vertexCount() const { return static_cast<uint32_t>(vertices.size()); }
    bool empty() const { return vertices.empty(); }

private:
    void destroy() noexcept;
    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                                   uint32_t typeFilter,
                                   VkMemoryPropertyFlags properties);

    VkDevice device = VK_NULL_HANDLE;
    VkBuffer vkBuffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    uint32_t maxVerts = 0;
    std::vector<LineVertex> vertices;
};
