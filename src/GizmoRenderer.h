#pragma once

#include <vulkan/vulkan.h>
#include <cstddef>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

class GizmoRenderer{
public:
    struct PushConstants{
        glm::mat4 view;
        glm::mat4 projection;
    };
    GizmoRenderer(VkDevice device, VkRenderPass renderPass);
    ~GizmoRenderer();

    GizmoRenderer(const GizmoRenderer&) = delete;
    GizmoRenderer& operator=(const GizmoRenderer&) = delete;
    GizmoRenderer(GizmoRenderer&&) noexcept;
    GizmoRenderer& operator=(GizmoRenderer&&) noexcept;

    VkPipeline getPipeline() const { return pipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
private:
    VkShaderModule createShaderModule(const void* bytes, size_t sizeBytes) const;
    void destroy() noexcept;

    VkDevice device = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
};


