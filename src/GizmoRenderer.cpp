#include "GizmoRenderer.h"
#include "Vertex.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace{
    std::vector<std::uint32_t> readFileWords(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file) throw std::runtime_error("Failed to open shader file: " + path.string());
        const std::streamsize size = file.tellg();
        if (size <= 0) throw std::runtime_error("Shader file is empty: " + path.string());
        if ((size % 4) != 0) throw std::runtime_error("Shader file size not multiple of 4: " + path.string());
        std::vector<std::uint8_t> bytes(static_cast<size_t>(size));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(bytes.data()), size);
        if (!file) throw std::runtime_error("Failed to read shader file: " + path.string());
        std::vector<std::uint32_t> words(bytes.size() / 4);
        std::memcpy(words.data(), bytes.data(), bytes.size());
        return words;
    }

std::filesystem::path shaderPath(const char* file) {
#ifndef VULKANLAB_SHADER_DIR
    (void)file;
    throw std::runtime_error("VULKANLAB_SHADER_DIR is not defined");
#else
    return std::filesystem::path(VULKANLAB_SHADER_DIR) / file;
#endif
}
} // namespace

GizmoRenderer::GizmoRenderer(VkDevice dev, VkRenderPass rp) : device(dev), renderPass(rp)
{
    if (device == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE)
        throw std::invalid_argument("GizmoRenderer: invalid Vulkan handles");

    VkShaderModule tmpVert = VK_NULL_HANDLE;
    VkShaderModule tmpFrag = VK_NULL_HANDLE;
    VkPipelineLayout tmpLayout = VK_NULL_HANDLE;
    VkPipeline tmpPipeline = VK_NULL_HANDLE;

    try{
        const auto vertWords = readFileWords(shaderPath("gizmo.vert.spv"));
        const auto fragWords = readFileWords(shaderPath("gizmo.frag.spv"));

        tmpVert = createShaderModule(vertWords.data(), vertWords.size() * sizeof(std::uint32_t));
        tmpFrag = createShaderModule(fragWords.data(), fragWords.size() * sizeof(std::uint32_t));

        VkPushConstantRange pcRange{};
        pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pcRange.offset = 0;
        pcRange.size = sizeof(PushConstants);

        VkPipelineLayoutCreateInfo pl{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pl.pushConstantRangeCount = 1;
        pl.pPushConstantRanges = &pcRange;

        if (vkCreatePipelineLayout(device, &pl, nullptr, &tmpLayout) != VK_SUCCESS)
            throw std::runtime_error("GizmoRenderer: vkCreatePipelineLayout failed");

        VkPipelineShaderStageCreateInfo vertStage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = tmpVert;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = tmpFrag;
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

        auto bindingDesc  = Vertex::bindingDescription();
        auto attribDescs  = Vertex::attributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &bindingDesc;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescs.size());
        vertexInput.pVertexAttributeDescriptions = attribDescs.data();

        VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vp.viewportCount = 1;
        vp.scissorCount  = 1;

        VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode    = VK_CULL_MODE_NONE; 
        rs.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth   = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        ds.depthTestEnable  = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState cba{};
        cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        cb.attachmentCount = 1;
        cb.pAttachments    = &cba;

        VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dyn.dynamicStateCount = 2;
        dyn.pDynamicStates    = dynStates;

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
        gp.layout = tmpLayout;
        gp.renderPass = renderPass;
        gp.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp, nullptr, &tmpPipeline) != VK_SUCCESS)
            throw std::runtime_error("GizmoRenderer: vkCreateGraphicsPipelines failed");

        vertModule    = tmpVert;
        fragModule    = tmpFrag;
        pipelineLayout = tmpLayout;
        pipeline       = tmpPipeline;
    } catch (...) {
        if (tmpPipeline) vkDestroyPipeline(device, tmpPipeline, nullptr);
        if (tmpLayout)   vkDestroyPipelineLayout(device, tmpLayout, nullptr);
        if (tmpVert)     vkDestroyShaderModule(device, tmpVert, nullptr);
        if (tmpFrag)     vkDestroyShaderModule(device, tmpFrag, nullptr);
        throw;
    }
}

void GizmoRenderer::destroy() noexcept {
    if (device != VK_NULL_HANDLE) {
        if (pipeline)      vkDestroyPipeline(device, pipeline, nullptr);
        if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (vertModule)    vkDestroyShaderModule(device, vertModule, nullptr);
        if (fragModule)    vkDestroyShaderModule(device, fragModule, nullptr);
    }
    pipeline = VK_NULL_HANDLE;
    pipelineLayout = VK_NULL_HANDLE;
    vertModule = VK_NULL_HANDLE;
    fragModule = VK_NULL_HANDLE;
    renderPass = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
}

GizmoRenderer::GizmoRenderer(GizmoRenderer&& o) noexcept
    : device(o.device), renderPass(o.renderPass),
      pipelineLayout(o.pipelineLayout), pipeline(o.pipeline),
      vertModule(o.vertModule), fragModule(o.fragModule)
{
    o.device = VK_NULL_HANDLE; o.renderPass = VK_NULL_HANDLE;
    o.pipelineLayout = VK_NULL_HANDLE; o.pipeline = VK_NULL_HANDLE;
    o.vertModule = VK_NULL_HANDLE; o.fragModule = VK_NULL_HANDLE;
}

GizmoRenderer& GizmoRenderer::operator=(GizmoRenderer&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device; renderPass = o.renderPass;
    pipelineLayout = o.pipelineLayout; pipeline = o.pipeline;
    vertModule = o.vertModule; fragModule = o.fragModule;
    o.device = VK_NULL_HANDLE; o.renderPass = VK_NULL_HANDLE;
    o.pipelineLayout = VK_NULL_HANDLE; o.pipeline = VK_NULL_HANDLE;
    o.vertModule = VK_NULL_HANDLE; o.fragModule = VK_NULL_HANDLE;
    return *this;
}

GizmoRenderer::~GizmoRenderer() {
    destroy();
}

VkShaderModule GizmoRenderer::createShaderModule(const void* bytes, size_t sizeBytes) const {
    if (bytes == nullptr || sizeBytes == 0 || (sizeBytes % 4) != 0)
        throw std::invalid_argument("Shader module bytes must be non-null and 4-byte aligned");
    VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    ci.codeSize = sizeBytes;
    ci.pCode    = reinterpret_cast<const uint32_t*>(bytes);
    VkShaderModule mod = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &ci, nullptr, &mod) != VK_SUCCESS)
        throw std::runtime_error("GizmoRenderer: vkCreateShaderModule failed");
    return mod;
}
