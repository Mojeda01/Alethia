#include "TriangleRenderer.h"
#include "../Vertex.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <array>

static_assert(sizeof(TriangleRenderer::PushConstants) == 96, "Push constants must be 96 bytes");

namespace{
std::vector<std::uint32_t> readFileWords(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open shader file: " + path.string());
    }

    const std::streamsize size = file.tellg();
    if (size <= 0) {
        throw std::runtime_error("Shader file is empty: " + path.string());
    }

    if ((size % 4) != 0) {
        throw std::runtime_error("Shader file size must be a multiple of 4: " + path.string());
    }

    std::vector<std::uint8_t> bytes(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()), size);
    if (!file) {
        throw std::runtime_error("Failed to read shader file: " + path.string());
    }

    std::vector<std::uint32_t> words(bytes.size() / 4);
    std::memcpy(words.data(), bytes.data(), bytes.size());
    return words;
}

std::filesystem::path shaderPath(const char* file) {
#ifndef VULKANLAB_SHADER_DIR
    (void)file;
    throw std::runtime_error("VULKANLAB_SHADER_DIR is not defined; shaders cannot be located");
}
#else
    return std::filesystem::path(VULKANLAB_SHADER_DIR) / file;
#endif
}
} // namespace

TriangleRenderer::TriangleRenderer(VkDevice dev, VkRenderPass rp, VkDescriptorSetLayout dsLayout)
    : device(dev), renderPass(rp)
{
    if (device == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE || dsLayout == VK_NULL_HANDLE) {
        throw std::invalid_argument("TriangleRenderer: invalid Vulkan handles");
    }

    VkShaderModule tmpVert = VK_NULL_HANDLE;
    VkShaderModule tmpFrag = VK_NULL_HANDLE;
    VkPipelineLayout tmpLayout = VK_NULL_HANDLE;
    VkPipeline tmpPipeline = VK_NULL_HANDLE;

    try {
        const auto vertWords = readFileWords(shaderPath("triangle.vert.spv"));
        const auto fragWords = readFileWords(shaderPath("triangle.frag.spv"));

        tmpVert = createShaderModule(vertWords.data(), vertWords.size() * sizeof(std::uint32_t));
        tmpFrag = createShaderModule(fragWords.data(), fragWords.size() * sizeof(std::uint32_t));

        VkPushConstantRange pcr{};
        pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pcr.offset = 0;
        pcr.size = sizeof(PushConstants);

        VkPipelineLayoutCreateInfo pl{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pl.setLayoutCount = 1;
        pl.pSetLayouts = &dsLayout;
        pl.pushConstantRangeCount = 1;
        pl.pPushConstantRanges = &pcr;

        if (vkCreatePipelineLayout(device, &pl, nullptr, &tmpLayout) != VK_SUCCESS) {
            throw std::runtime_error("vkCreatePipelineLayout failed");
        }

        VkPipelineShaderStageCreateInfo vertStage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = tmpVert;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = tmpFrag;
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

        auto bindingDesc = Vertex::bindingDescription();
        auto attribDescs = Vertex::attributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &bindingDesc;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescs.size());
        vertexInput.pVertexAttributeDescriptions = attribDescs.data();

        VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vp.viewportCount = 1;
        vp.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE;
        rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = VK_TRUE;
        ds.depthCompareOp = VK_COMPARE_OP_LESS;
        ds.depthBoundsTestEnable = VK_FALSE;
        ds.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState cba{};
        cba.colorWriteMask =    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        cb.attachmentCount = 1;
        cb.pAttachments = &cba;

        VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dyn.dynamicStateCount = static_cast<uint32_t>(sizeof(dynStates) / sizeof(dynStates[0]));
        dyn.pDynamicStates = dynStates;

        VkPipelineCache cache = VK_NULL_HANDLE;
        VkGraphicsPipelineCreateInfo gp{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        gp.stageCount = 2;
        gp.pStages = stages;
        gp.pVertexInputState = &vertexInput;
        gp.pInputAssemblyState = &ia;
        gp.pViewportState = &vp;
        gp.pRasterizationState = &rs;
        gp.pMultisampleState = &ms;
        gp.pDepthStencilState = &ds;
        gp.pColorBlendState = &cb;
        gp.pDynamicState = &dyn;
        gp.pTessellationState = nullptr;
        gp.layout = tmpLayout;
        gp.renderPass = renderPass;
        gp.subpass = 0;

        if (vkCreateGraphicsPipelines(device, cache, 1, &gp, nullptr, &tmpPipeline) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateGraphicsPipeline failed");
        }

        rs.polygonMode = VK_POLYGON_MODE_LINE;
        rs.lineWidth = 1.0f;

        VkPipeline tmpWireframe = VK_NULL_HANDLE;
        if (vkCreateGraphicsPipelines(device, cache, 1, &gp, nullptr, &tmpWireframe) != VK_SUCCESS) {
            vkDestroyPipeline(device, tmpPipeline, nullptr);
            throw std::runtime_error("vkCreateGraphicsPipeline (wireframe) failed");
        }

        pipelineLayout = tmpLayout;
        pipeline = tmpPipeline;
        wireframePipeline = tmpWireframe;
        vkDestroyShaderModule(device, tmpVert, nullptr);
        vkDestroyShaderModule(device, tmpFrag, nullptr);
        tmpVert = VK_NULL_HANDLE;
        tmpFrag = VK_NULL_HANDLE;
    } catch (...) {
        if (tmpPipeline) vkDestroyPipeline(device, tmpPipeline, nullptr);
        if (tmpLayout) vkDestroyPipelineLayout(device, tmpLayout, nullptr);
        if (tmpVert) vkDestroyShaderModule(device, tmpVert, nullptr);
        if (tmpFrag) vkDestroyShaderModule(device, tmpFrag, nullptr);
        throw;
    }
}

void TriangleRenderer::destroy() noexcept{
    if (device != VK_NULL_HANDLE) {
        if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        if (wireframePipeline) vkDestroyPipeline(device, wireframePipeline, nullptr);
        if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr); 
    }
    pipeline = VK_NULL_HANDLE;
    wireframePipeline = VK_NULL_HANDLE;
    pipelineLayout = VK_NULL_HANDLE; 
    renderPass = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
}

TriangleRenderer::TriangleRenderer(TriangleRenderer&& o) noexcept
        :   device(o.device),
            renderPass(o.renderPass),
            pipelineLayout(o.pipelineLayout),
            pipeline(o.pipeline),
            wireframePipeline(o.wireframePipeline){
    o.device = VK_NULL_HANDLE;
    o.renderPass = VK_NULL_HANDLE;
    o.pipelineLayout = VK_NULL_HANDLE;
    o.pipeline = VK_NULL_HANDLE;
    o.wireframePipeline = VK_NULL_HANDLE;  
}

TriangleRenderer& TriangleRenderer::operator=(TriangleRenderer&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    renderPass = o.renderPass;
    pipelineLayout = o.pipelineLayout;
    pipeline = o.pipeline;
    wireframePipeline = o.wireframePipeline; 
    o.device = VK_NULL_HANDLE;
    o.renderPass = VK_NULL_HANDLE;
    o.pipelineLayout = VK_NULL_HANDLE;
    o.pipeline = VK_NULL_HANDLE; 
    return *this;
}

TriangleRenderer::~TriangleRenderer() {
    destroy();
}

VkShaderModule TriangleRenderer::createShaderModule(const void* bytes, size_t sizeBytes) const {
    if (bytes == nullptr || sizeBytes == 0 || (sizeBytes % 4) != 0) {
        throw std::invalid_argument("Shader module bytes must be non-null and 4-byte aligned");
    }

    VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    ci.codeSize = sizeBytes;
    ci.pCode = reinterpret_cast<const uint32_t*>(bytes);

    VkShaderModule mod = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &ci, nullptr, &mod) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateShaderModule failed");
    }
    return mod;
}

void TriangleRenderer::record(  VkCommandBuffer cmd,
                                VkFramebuffer framebuffer,
                                VkExtent2D extent,
                                VkBuffer vertexBuffer,
                                uint32_t vertexCount,
                                VkDescriptorSet descriptorSet,
                                const PushConstants& pushConstants,
                                VkClearColorValue clearColor) const {

    if (cmd == VK_NULL_HANDLE || framebuffer == VK_NULL_HANDLE || descriptorSet == VK_NULL_HANDLE) {
        throw std::invalid_argument("TriangleRenderer::record: invalid Vulkan handles");
    }

    if (vertexCount == 0) {
        throw std::invalid_argument("TriangleRenderer::record: vertexCount must be greater than zero");
    }

    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rp{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rp.renderPass = renderPass;
    rp.framebuffer = framebuffer;
    rp.renderArea.offset = { 0, 0 };
    rp.renderArea.extent = extent;
    rp.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rp.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                            0, 1, &descriptorSet, 0, nullptr);

    vkCmdPushConstants( cmd,
                        pipelineLayout,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0,
                        sizeof(PushConstants),
                        &pushConstants);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdDraw(cmd, vertexCount, 1, 0, 0);
    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
}

void TriangleRenderer::recordIndexed(
                                VkCommandBuffer cmd,
                                VkFramebuffer framebuffer,
                                VkExtent2D extent,
                                VkBuffer vertexBuffer,
                                VkBuffer indexBuffer,
                                uint32_t indexCount,
                                VkDescriptorSet descriptorSet,
                                const PushConstants& pushConstants,
                                VkClearColorValue clearColor) const {

    if (cmd == VK_NULL_HANDLE || framebuffer == VK_NULL_HANDLE || descriptorSet == VK_NULL_HANDLE) {
        throw std::invalid_argument("TriangleRenderer::recordIndexed: invalid Vulkan handles");
    }

    if (indexCount == 0) {
        throw std::invalid_argument("TriangleRenderer::recordIndexed: indexCount must be greater than zero");
    }

    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rp{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rp.renderPass = renderPass;
    rp.framebuffer = framebuffer;
    rp.renderArea.offset = { 0, 0 };
    rp.renderArea.extent = extent;
    rp.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rp.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                            0, 1, &descriptorSet, 0, nullptr);

    vkCmdPushConstants( cmd,
                        pipelineLayout,
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                        0,
                        sizeof(PushConstants),
                        &pushConstants);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);
    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
}
