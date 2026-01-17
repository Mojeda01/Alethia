#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class FramebufferSet{
public:
    FramebufferSet(VkDevice device,
                    VkRenderPass renderPass,
                    VkExtent2D extent,
                    const std::vector<VkImageView>& imageView);
    ~FramebufferSet();
    const std::vector<VkFramebuffer>& get() const;
private:
    VkDevice device;
    std::vector<VkFramebuffer> framebuffers;
};
