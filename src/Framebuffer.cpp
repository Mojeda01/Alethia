#include "Framebuffer.h"
#include <stdexcept>

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
    for (auto fb : framebuffers) {
        vkDestroyFramebuffer(device, fb, nullptr);
    }
}

const std::vector<VkFramebuffer>& FramebufferSet::get() const {
    return framebuffers;
}
