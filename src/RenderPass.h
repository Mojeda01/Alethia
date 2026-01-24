#pragma once
#include <vulkan/vulkan.h>

class RenderPass{
public:
    RenderPass(VkDevice device, VkFormat colorFormat);
    ~RenderPass();
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&) noexcept;
    RenderPass& operator=(RenderPass&&) noexcept;

    VkRenderPass get() const;
private:
    void destroy() noexcept;
    VkDevice device;
    VkRenderPass renderPass = VK_NULL_HANDLE;
};
