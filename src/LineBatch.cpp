#include "LineBatch.h"

#include <cstring>
#include <stdexcept>

uint32_t LineBatch::findMemoryType(VkPhysicalDevice physicalDevice,
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
    throw std::runtime_error("LineBatch: failed to find suitable memory type");
}

LineBatch::LineBatch(VkDevice dev, VkPhysicalDevice physicalDevice,
                     VkCommandPool commandPool, VkQueue transferQueue,
                     uint32_t maxVertices)
    : device(dev), maxVerts(maxVertices)
{
    (void)commandPool;
    (void)transferQueue;

    VkDeviceSize bufferSize = sizeof(LineVertex) * maxVerts;

    VkBufferCreateInfo bci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bci.size = bufferSize;
    bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bci, nullptr, &vkBuffer) != VK_SUCCESS) {
        throw std::runtime_error("LineBatch: vkCreateBuffer failed");
    }

    VkMemoryRequirements memReqs{};
    vkGetBufferMemoryRequirements(device, vkBuffer, &memReqs);

    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = memReqs.size;
    ai.memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &ai, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(device, vkBuffer, nullptr);
        vkBuffer = VK_NULL_HANDLE;
        throw std::runtime_error("LineBatch: vkAllocateMemory failed");
    }

    if (vkBindBufferMemory(device, vkBuffer, memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("LineBatch: vkBindBufferMemory failed");
    }

    if (vkMapMemory(device, memory, 0, bufferSize, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("LineBatch: vkMapMemory failed");
    }

    vertices.reserve(maxVerts);
}

void LineBatch::clear() {
    vertices.clear();
}

void LineBatch::addLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& color) {
    if (vertices.size() + 2 > maxVerts) return;

    LineVertex v0{};
    v0.position[0] = p0.x; v0.position[1] = p0.y; v0.position[2] = p0.z;
    v0.color[0] = color.x; v0.color[1] = color.y; v0.color[2] = color.z;

    LineVertex v1{};
    v1.position[0] = p1.x; v1.position[1] = p1.y; v1.position[2] = p1.z;
    v1.color[0] = color.x; v1.color[1] = color.y; v1.color[2] = color.z;

    vertices.push_back(v0);
    vertices.push_back(v1);
}

void LineBatch::addAABBEdges(const glm::vec3& min, const glm::vec3& max,
                             const glm::vec3& colorX, const glm::vec3& colorY, const glm::vec3& colorZ) {
    glm::vec3 corners[8] = {
        { min.x, min.y, min.z },
        { max.x, min.y, min.z },
        { max.x, min.y, max.z },
        { min.x, min.y, max.z },
        { min.x, max.y, min.z },
        { max.x, max.y, min.z },
        { max.x, max.y, max.z },
        { min.x, max.y, max.z },
    };

    float t = 0.005f;
    glm::vec3 offsets[] = {
        {0, 0, 0}, {0, t, 0}, {0, -t, 0}, {0, 0, t}, {0, 0, -t},
        {t, 0, 0}, {-t, 0, 0}, {t, t, 0}, {-t, -t, 0}
    };

    for (const auto& off : offsets){
        addLine(corners[0]+off, corners[1]+off, colorX);
        addLine(corners[2]+off, corners[3]+off, colorX);
        addLine(corners[4]+off, corners[5]+off, colorX);
        addLine(corners[6]+off, corners[7]+off, colorX);

        addLine(corners[0]+off, corners[4]+off, colorY);
        addLine(corners[1]+off, corners[5]+off, colorY);
        addLine(corners[2]+off, corners[6]+off, colorY);
        addLine(corners[2]+off, corners[6]+off, colorY);

        addLine(corners[0]+off, corners[3]+off, colorZ);
        addLine(corners[1]+off, corners[2]+off, colorZ);
        addLine(corners[4]+off, corners[7]+off, colorZ);
        addLine(corners[5]+off, corners[6]+off, colorZ);
    }

}

void LineBatch::upload() {
    if (!vertices.empty() && mapped) {
        std::memcpy(mapped, vertices.data(), vertices.size() * sizeof(LineVertex));
    }
}

void LineBatch::destroy() noexcept {
    if (device != VK_NULL_HANDLE) {
        if (mapped && memory != VK_NULL_HANDLE) {
            vkUnmapMemory(device, memory);
        }
        if (vkBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, vkBuffer, nullptr);
        if (memory != VK_NULL_HANDLE) vkFreeMemory(device, memory, nullptr);
    }
    mapped = nullptr;
    vkBuffer = VK_NULL_HANDLE;
    memory = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
}

LineBatch::LineBatch(LineBatch&& o) noexcept
    : device(o.device), vkBuffer(o.vkBuffer), memory(o.memory),
      mapped(o.mapped), maxVerts(o.maxVerts), vertices(std::move(o.vertices))
{
    o.device = VK_NULL_HANDLE;
    o.vkBuffer = VK_NULL_HANDLE;
    o.memory = VK_NULL_HANDLE;
    o.mapped = nullptr;
}

LineBatch& LineBatch::operator=(LineBatch&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    vkBuffer = o.vkBuffer;
    memory = o.memory;
    mapped = o.mapped;
    maxVerts = o.maxVerts;
    vertices = std::move(o.vertices);
    o.device = VK_NULL_HANDLE;
    o.vkBuffer = VK_NULL_HANDLE;
    o.memory = VK_NULL_HANDLE;
    o.mapped = nullptr;
    return *this;
}

LineBatch::~LineBatch() {
    destroy();
}
