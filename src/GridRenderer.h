#pragma once

#include <vulkan/vulkan.h>
#include <cstddef>

class GridRenderer{
public:
    GridRenderer(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
    ~GridRenderer();

    GridRenderer(const GridRenderer&) = delete;
    GridRenderer& operator=(const GridRenderer&) = delete;
    GridRenderer(GridRenderer&&) noexcept;
    GridRenderer& operator=(GridRenderer&&) noexcept;

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
