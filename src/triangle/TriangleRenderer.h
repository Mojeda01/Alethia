#pragma once

#include <vulkan/vulkan.h>
#include <cstddef>
#include <cstdint>

class TriangleRenderer {
public:
    TriangleRenderer(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
    ~TriangleRenderer();

    TriangleRenderer(const TriangleRenderer&) = delete;
    TriangleRenderer& operator=(const TriangleRenderer&) = delete;
    TriangleRenderer(TriangleRenderer&&) noexcept;
    TriangleRenderer& operator=(TriangleRenderer&&) noexcept;
    struct PushConstants{
        float timeSeconds = 0.0f;
        float deltaSeconds = 0.0f;
        std::uint32_t frameIndex = 0;
        float pad = 0.0f;
    };

    void record(    VkCommandBuffer cmd,
                    VkFramebuffer framebuffer,
                    VkExtent2D extent,
                    VkBuffer vertexBuffer,
                    uint32_t vertexCount,
                    VkDescriptorSet descriptorSet,
                    const PushConstants& pushConstants,
                    VkClearColorValue clearColor = { { 0.05f, 0.05f, 0.05f, 1.0f } }) const;

    void recordIndexed( VkCommandBuffer cmd,
                        VkFramebuffer framebuffer,
                        VkExtent2D extent,
                        VkBuffer vertexBuffer,
                        VkBuffer indexBuffer,
                        uint32_t indexCount,
                        VkDescriptorSet descriptorSet,
                        const PushConstants& pushConstants,
                        VkClearColorValue clearColor = { { 0.05f, 0.05f, 0.05f, 1.0f } }) const;
    VkPipeline getPipeline() const { return pipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

private:
    VkShaderModule createShaderModule(const void* bytes, size_t sizeBytes) const;
private:
    void destroy() noexcept;
    VkDevice device = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
};
