#include "RenderPass.h"
#include <stdexcept>
#include <utility>

RenderPass::RenderPass(VkDevice dev, VkFormat colorFormat) : device(dev) {
    VkAttachmentDescription color{};
    color.format = colorFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo ci{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    ci.attachmentCount = 1;
    ci.pAttachments = &color;
    ci.subpassCount = 1;
    ci.pSubpasses = &subpass;
    ci.dependencyCount = 1;
    ci.pDependencies = &dep;

    if (vkCreateRenderPass(device, &ci, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("RenderPass creation failed");
    }
}

RenderPass::~RenderPass() {
    destroy();
}

void RenderPass::destroy() noexcept {
    if (device != VK_NULL_HANDLE && renderPass) {
        vkDestroyRenderPass(device, renderPass, nullptr);
    }
    renderPass = VK_NULL_HANDLE;
}

RenderPass::RenderPass(RenderPass&& o) noexcept : device(o.device),
    renderPass(o.renderPass) {
    o.device = VK_NULL_HANDLE;
    o.renderPass = VK_NULL_HANDLE;
}

RenderPass& RenderPass::operator=(RenderPass&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    renderPass = o.renderPass;
    o.device = VK_NULL_HANDLE;
    o.renderPass = VK_NULL_HANDLE;
    return *this;
}

VkRenderPass RenderPass::get() const {
    return renderPass;
}
