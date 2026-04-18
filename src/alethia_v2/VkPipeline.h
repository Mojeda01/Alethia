#pragma once 

#include <vulkan/vulkan.h>
#include <string>

struct Pipeline{
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline handle = VK_NULL_HANDLE;
};

// Create the triangle graphics pipeline
Pipeline createTrianglePipeline(
    VkDevice device,
    VkFormat colorFormat,
    VkFormat depthFormat,
    const std::string& vertSpvPath,
    const std::string& fragSpvPath
);

void destroyPipeline(VkDevice device, Pipeline& pipeline);

// hot-reload helper
bool reloadTrianglePipeline(
    VkDevice device,
    Pipeline& pipeline,
    VkFormat colorFormat,
    VkFormat depthFormat,
    const std::string& vertSpvPath,
    const std::string& fragSpvPath
);
