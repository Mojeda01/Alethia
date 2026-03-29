#pragma once

#include "Mesh.h"
#include "MeshInstance.h"
#include "Device.h"
#include "MeshBuffer.h"
#include "UniformBuffer.h"
#include "CommandPool.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <vulkan/vulkan.h>

class MeshRenderer {
public:
    MeshRenderer(Device& device, 
                 CommandPool& commandPool, 
                 VkQueue transferQueue,
                 VkRenderPass renderPass, 
                 VkDescriptorSetLayout uniformLayout);

    ~MeshRenderer();

    uint32_t loadMesh(const Mesh& mesh, const std::string& name);
    void addInstance(uint32_t meshId, const glm::mat4& transform, const glm::vec3& color = glm::vec3(1.0f));
    void record(VkCommandBuffer cmd, const UniformBuffer& uniformBuffer, uint32_t imageIndex);
    void clearInstances();

private:
    Device& device;
    CommandPool& commandPool;
    VkQueue transferQueue;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    struct LoadedMesh {
        MeshBuffer buffer;
        uint32_t indexCount = 0;
    };

    std::vector<LoadedMesh> loadedMeshes;
    std::unordered_map<std::string, uint32_t> nameToId;

    struct InstanceData {
        glm::mat4 transform;
        glm::vec3 color;
        uint32_t meshId;
    };

    std::vector<InstanceData> instancesThisFrame;
};
