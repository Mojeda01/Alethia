#include "Framebuffer.h"
#include <stdexcept>
#include <utility>

FramebufferSet::FramebufferSet(VkDevice dev,
                                VkRenderPass renderPass,
                                VkExtent2D extent,
                                const std::vector<VkImageView>& imageViews) 
    : device(dev) 
{
    framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); ++i) {
        VkImageView attachments[] = { imageViews[i] };

        VkFramebufferCreateInfo ci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        ci.renderPass = renderPass;
        ci.attachmentCount = 1;
        ci.pAttachments = attachments;
        ci.width = extent.width;
        ci.height = extent.height;
        ci.layers = 1;

        if (vkCreateFramebuffer(device, &ci, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Framebuffer creation failed");
        }
    } 
}

FramebufferSet::~FramebufferSet() {
    destroy();
}

void FramebufferSet::destroy() noexcept {
    if (device != VK_NULL_HANDLE) {
        for (auto fb : framebuffers) {
            if (fb) vkDestroyFramebuffer(device, fb, nullptr);
        }
    }
    framebuffers.clear();
}

FramebufferSet::FramebufferSet(FramebufferSet&& o) noexcept
    : device(o.device), framebuffers(std::move(o.framebuffers)) {
    o.device = VK_NULL_HANDLE;
}

FramebufferSet& FramebufferSet::operator=(FramebufferSet&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    framebuffers = std::move(o.framebuffers);
    o.device = VK_NULL_HANDLE;
    return *this;
}

const std::vector<VkFramebuffer>& FramebufferSet::get() const {
    return framebuffers;
}
