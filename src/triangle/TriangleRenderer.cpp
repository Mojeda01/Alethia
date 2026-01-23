#include "TriangleRenderer.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <vector>

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

TriangleRenderer::TriangleRenderer(VkDevice dev, VkRenderPass rp) : device(dev), renderPass(rp) {
    if (device == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE) {
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

        VkPipelineLayoutCreateInfo pl{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
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

    VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

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
    gp.pDepthStencilState = nullptr;
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &dyn;
    gp.pTessellationState = nullptr;
    gp.layout = tmpLayout;
    gp.renderPass = renderPass;
    gp.subpass = 0;
        
        if (vkCreateGraphicsPipelines(device, cache, 1, &gp, nullptr, &tmpPipeline) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateGraphicsPipeline failed");
        }

        vertModule = tmpVert;
        fragModule = tmpFrag;
        pipelineLayout = tmpLayout;
        pipeline = tmpPipeline;
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
        if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (vertModule) vkDestroyShaderModule(device, vertModule, nullptr);
        if (fragModule) vkDestroyShaderModule(device, fragModule, nullptr);
    }
    pipeline = VK_NULL_HANDLE;
    pipelineLayout = VK_NULL_HANDLE;
    vertModule = VK_NULL_HANDLE;
    fragModule = VK_NULL_HANDLE;
    renderPass = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
}

TriangleRenderer::TriangleRenderer(TriangleRenderer&& o) noexcept
        :   device(o.device),
            renderPass(o.renderPass),
            pipelineLayout(o.pipelineLayout),
            pipeline(o.pipeline),
            vertModule(o.vertModule),
            fragModule(o.fragModule){
    o.device = VK_NULL_HANDLE;
    o.renderPass = VK_NULL_HANDLE;
    o.pipelineLayout = VK_NULL_HANDLE;
    o.pipeline = VK_NULL_HANDLE;
    o.vertModule = VK_NULL_HANDLE;
    o.fragModule = VK_NULL_HANDLE;
}

TriangleRenderer& TriangleRenderer::operator=(TriangleRenderer&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    renderPass = o.renderPass;
    pipelineLayout = o.pipelineLayout;
    pipeline = o.pipeline;
    vertModule = o.vertModule;
    fragModule = o.fragModule;
    o.device = VK_NULL_HANDLE;
    o.renderPass = VK_NULL_HANDLE;
    o.pipelineLayout = VK_NULL_HANDLE;
    o.pipeline = VK_NULL_HANDLE;
    o.vertModule = VK_NULL_HANDLE;
    o.fragModule = VK_NULL_HANDLE;
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
                                VkClearColorValue clearColor) const {

    if (cmd == VK_NULL_HANDLE || framebuffer == VK_NULL_HANDLE) {
        throw std::invalid_argument("TriangleRenderer::record: invalid Vulkan handles");
    } 

    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    VkClearValue clear{};
    clear.color = clearColor;

    VkRenderPassBeginInfo rp{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO }; 
    rp.renderPass = renderPass;
    rp.framebuffer = framebuffer;
    rp.renderArea.offset = { 0, 0 };
    rp.renderArea.extent = extent;
    rp.clearValueCount = 1;
    rp.pClearValues = &clear;

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

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

    vkCmdDraw(cmd, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
}
