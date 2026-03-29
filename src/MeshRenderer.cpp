#include "MeshRenderer.h"
#include <fstream>
#include <vector>
#include <stdexcept>

MeshRenderer::MeshRenderer(Device& device, 
                           CommandPool& commandPool, 
                           VkQueue transferQueue,
                           VkRenderPass renderPass, 
                           VkDescriptorSetLayout uniformLayout)
    : device(device)
    , commandPool(commandPool)
    , transferQueue(transferQueue)
{
    if (device.get() == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE || uniformLayout == VK_NULL_HANDLE) {
        throw std::invalid_argument("MeshRenderer: invalid Vulkan handles provided");
    }

    VkShaderModule vertShader = VK_NULL_HANDLE;
    VkShaderModule fragShader = VK_NULL_HANDLE;
    VkPipelineLayout tmpLayout = VK_NULL_HANDLE;
    VkPipeline tmpPipeline = VK_NULL_HANDLE;

    try {
        std::string shaderDir = VULKANLAB_SHADER_DIR;
        auto readFile = [](const std::string& filename) -> std::vector<char> {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open shader file: " + filename);
            }
            size_t size = static_cast<size_t>(file.tellg());
            std::vector<char> buffer(size);
            file.seekg(0);
            file.read(buffer.data(), size);
            return buffer;
        };
        
        auto vertCode = readFile(shaderDir + "/mesh.vert.spv");
        auto fragCode = readFile(shaderDir + "/mesh.frag.spv");

        // Create shader modules
        auto createShaderModule = [&](const std::vector<char>& code) -> VkShaderModule {
            VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            ci.codeSize = code.size();
            ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
            VkShaderModule mod;
            if (vkCreateShaderModule(device.get(), &ci, nullptr, &mod) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create shader module");
            }
            return mod;
        };

        vertShader = createShaderModule(vertCode);
        fragShader = createShaderModule(fragCode);

        // Push constants: model + color
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4) + sizeof(glm::vec3);

        VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &uniformLayout;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(device.get(), &layoutInfo, nullptr, &tmpLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create MeshRenderer pipeline layout");
        }

        // Shader stages
        VkPipelineShaderStageCreateInfo vertStage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertShader;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragShader;
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

        // Vertex input
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0].binding = 0; attributeDescriptions[0].location = 0; attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; attributeDescriptions[0].offset = offsetof(Vertex, position);
        attributeDescriptions[1].binding = 0; attributeDescriptions[1].location = 1; attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; attributeDescriptions[1].offset = offsetof(Vertex, color);
        attributeDescriptions[2].binding = 0; attributeDescriptions[2].location = 2; attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT; attributeDescriptions[2].offset = offsetof(Vertex, normal);
        attributeDescriptions[3].binding = 0; attributeDescriptions[3].location = 3; attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;   attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = 4;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = tmpLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device.get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &tmpPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create MeshRenderer graphics pipeline");
        }

        pipelineLayout = tmpLayout;
        pipeline = tmpPipeline;

        vkDestroyShaderModule(device.get(), vertShader, nullptr);
        vkDestroyShaderModule(device.get(), fragShader, nullptr);

    } catch (...) {
        if (tmpPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device.get(), tmpPipeline, nullptr);
        if (tmpLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device.get(), tmpLayout, nullptr);
        if (vertShader != VK_NULL_HANDLE) vkDestroyShaderModule(device.get(), vertShader, nullptr);
        if (fragShader != VK_NULL_HANDLE) vkDestroyShaderModule(device.get(), fragShader, nullptr);
        throw;
    }
}

MeshRenderer::~MeshRenderer()
{
    if (device.get() == VK_NULL_HANDLE) return;

    if (pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device.get(), pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device.get(), pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }

    loadedMeshes.clear();
    nameToId.clear();
    instancesThisFrame.clear();
}

uint32_t MeshRenderer::loadMesh(const Mesh& mesh, const std::string& name)
{
    if (nameToId.count(name)) {
        return nameToId[name];
    }

    // Create LoadedMesh with proper initialization since MeshBuffer has no default ctor
    LoadedMesh lm { MeshBuffer(device.get(), device.physical(), commandPool.get(), transferQueue, mesh.vertices, mesh.indices),
        static_cast<uint32_t>(mesh.indices.size()) };

    uint32_t id = static_cast<uint32_t>(loadedMeshes.size());
    loadedMeshes.push_back(std::move(lm));
    nameToId[name] = id;
    return id;
}

void MeshRenderer::addInstance(uint32_t meshId, const glm::mat4& transform, const glm::vec3& color)
{
    if (meshId >= loadedMeshes.size()) return;
    instancesThisFrame.push_back({transform, color, meshId});
}

void MeshRenderer::record(VkCommandBuffer cmd, const UniformBuffer& uniformBuffer, uint32_t imageIndex)
{
    if (instancesThisFrame.empty() || pipeline == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDescriptorSet uboSet = uniformBuffer.descriptorSet(imageIndex);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uboSet, 0, nullptr);

    for (const auto& inst : instancesThisFrame) {
        const LoadedMesh& m = loadedMeshes[inst.meshId];

        VkBuffer vb[] = { m.buffer.vertexBuffer() };
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, vb, &offset);

        if (m.indexCount > 0) {
            vkCmdBindIndexBuffer(cmd, m.buffer.indexBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }

        struct PushData {
            glm::mat4 model;
            glm::vec3 color;
        } push = { inst.transform, inst.color };

        vkCmdPushConstants(cmd, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(PushData), &push);

        if (m.indexCount > 0) {
            vkCmdDrawIndexed(cmd, m.indexCount, 1, 0, 0, 0);
        } else {
            vkCmdDraw(cmd, m.buffer.vertexCount(), 1, 0, 0);
        }
    }
}

void MeshRenderer::clearInstances()
{
    instancesThisFrame.clear();
}
