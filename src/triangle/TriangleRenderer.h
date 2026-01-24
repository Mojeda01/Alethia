#pragma once

#include <vulkan/vulkan.h>
#include <cstddef>

class TriangleRenderer {
public:
    TriangleRenderer(VkDevice device, VkRenderPass renderPass);
    ~TriangleRenderer();

    TriangleRenderer(const TriangleRenderer&) = delete;
    TriangleRenderer& operator=(const TriangleRenderer&) = delete;
    TriangleRenderer(TriangleRenderer&&) noexcept;
    TriangleRenderer& operator=(TriangleRenderer&&) noexcept;

    void record(    VkCommandBuffer cmd,
                    VkFramebuffer framebuffer,
                    VkExtent2D extent,
                    VkClearColorValue clearColor = { { 0.05f, 0.05f, 0.05f, 1.0f } }) const;
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
