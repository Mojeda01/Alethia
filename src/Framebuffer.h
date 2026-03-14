#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class FramebufferSet{
public:
    FramebufferSet(VkDevice device,
                    VkRenderPass renderPass,
                    VkExtent2D extent,
                    const std::vector<VkImageView>& imageViews,
                    VkImageView depthView);
    ~FramebufferSet();
    FramebufferSet(const FramebufferSet&) = delete;
    FramebufferSet& operator=(const FramebufferSet&) = delete;
    FramebufferSet(FramebufferSet&&) noexcept;
    FramebufferSet& operator=(FramebufferSet&&) noexcept;
    const std::vector<VkFramebuffer>& get() const;
private:
    void destroy() noexcept;
    VkDevice device;
    std::vector<VkFramebuffer> framebuffers;
};
