#pragma once
#include <vulkan/vulkan.h>

class RenderPass{
public:
    RenderPass(VkDevice device, VkFormat colorFormat);
    ~RenderPass();

    VkRenderPass get() const;
private:
    VkDevice device;
    VkRenderPass renderPass = VK_NULL_HANDLE;
};
