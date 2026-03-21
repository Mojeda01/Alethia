#pragma once

#include <vulkan/vulkan.h>
#include <cstddef>

class LineRenderer {
public:
    LineRenderer(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
    ~LineRenderer();

    LineRenderer(const LineRenderer&) = delete;
    LineRenderer& operator=(const LineRenderer&) = delete;
    LineRenderer(LineRenderer&&) noexcept;
    LineRenderer& operator=(LineRenderer&&) noexcept;

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
