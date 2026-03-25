#include "SceneRenderer.h"
#include "Log.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <stdexcept>

SceneRenderer::SceneRenderer(
    VkDevice          device,
    VkPhysicalDevice  physical,
    VkCommandPool     commandPool,
    VkQueue           graphicsQueue,
    SwapchainBundle&  swapchainBundle,
    UniformBuffer&    uniformBuffer,
    TriangleRenderer& triangle,
    GridRenderer&     grid,
    LineRenderer&     lineRenderer,
    GizmoRenderer&    gizmo,
    MeshBuffer&       cubeMesh,
    MeshBuffer&       gridMesh,
    MeshBuffer&       gizmoMesh,
    LineBatch&        lineBatch,
    ImGuiLayer&       imgui)
    : device(device)
    , physical(physical)
    , commandPool(commandPool)
    , graphicsQueue(graphicsQueue)
    , swapchainBundle(swapchainBundle)
    , uniformBuffer(uniformBuffer)
    , triangle(triangle)
    , grid(grid)
    , lineRenderer(lineRenderer)
    , gizmo(gizmo)
    , cubeMesh(cubeMesh)
    , gridMesh(gridMesh)
    , gizmoMesh(gizmoMesh)
    , lineBatch(lineBatch)
    , imgui(imgui)
{}

void SceneRenderer::rebuildPrismCache(const std::vector<SceneObject>& objects) {
    prismCache.clear();
    for (const auto& obj : objects) {
        if (obj.type == ShapeType::Prism) {
            PrismGeometry geo = makePrismMesh(obj.prism);
            if (!geo.vertices.empty()) {
                prismCache.push_back(std::make_unique<MeshBuffer>(
                    device, physical, commandPool, graphicsQueue,
                    geo.vertices, geo.indices));
            } else {
                prismCache.push_back(nullptr);
            }
        } else {
            prismCache.push_back(nullptr);
        }
    }
    prismCacheDirty = false;
}

void SceneRenderer::rebuildPrismCacheIfNeeded(const std::vector<SceneObject>& objects)
{
    if (prismCacheDirty) {
        vkDeviceWaitIdle(device);
        rebuildPrismCache(objects);
    }
}

void SceneRenderer::record(VkCommandBuffer cmd, const DrawContext& ctx) {
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }
    
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.12f, 0.12f, 0.14f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    
    VkExtent2D ext = swapchainBundle.extent();
    
    VkRenderPassBeginInfo rp{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rp.renderPass = swapchainBundle.renderPass();
    rp.framebuffer = swapchainBundle.framebuffers()[ctx.imageIndex];
    rp.renderArea.offset = { 0, 0 };
    rp.renderArea.extent = ext;
    rp.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rp.pClearValues = clearValues.data();
    
    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
    
    VkViewport viewport{};
    viewport.width = static_cast<float>(ext.width);
    viewport.height = static_cast<float>(ext.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.extent = ext;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    
    drawGrid(cmd, ctx.imageIndex);
    drawHighlight(cmd, ctx);
    drawPreview(cmd, ctx);
    drawObjects(cmd, ctx);
    drawSelectionLines(cmd, ctx);
    drawGizmo(cmd, ctx);
    
    imgui.render(cmd);
    
    vkCmdEndRenderPass(cmd);
    
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
}

void SceneRenderer::drawGrid(VkCommandBuffer cmd, uint32_t imageIndex) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, grid.getPipeline());
    VkDescriptorSet ds = uniformBuffer.descriptorSet(imageIndex);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            grid.getPipelineLayout(), 0, 1, &ds, 0, nullptr);
    VkDeviceSize offset = 0;
    VkBuffer gridVb = gridMesh.vertexBuffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &gridVb, &offset);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void SceneRenderer::drawHighlight(VkCommandBuffer cmd, const DrawContext& ctx)
{
    if (!ctx.editor.hasHighlight()) return;
    if (ctx.editor.activeTool() != SceneEditor::Tool::Place) return;
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle.getPipeline());
    VkDescriptorSet sceneDs = uniformBuffer.descriptorSet(ctx.imageIndex);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            triangle.getPipelineLayout(), 0, 1, &sceneDs, 0, nullptr);
    vkCmdPushConstants(cmd, triangle.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(TriangleRenderer::PushConstants), &ctx.pushConstants);
    VkDeviceSize cubeOffset = 0;
    VkBuffer cubeVb = cubeMesh.vertexBuffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &cubeVb, &cubeOffset);
    vkCmdBindIndexBuffer(cmd, cubeMesh.indexBuffer(), 0, VK_INDEX_TYPE_UINT32);
    
    glm::vec3 hlMin = ctx.editor.highlightMin();
    glm::vec3 hlMax = ctx.editor.highlightMax();
    glm::vec3 hlCenter = (hlMin + hlMax) * 0.5f;
    glm::vec3 hlSize = hlMax - hlMin;
    if (hlSize.y < 0.02f) hlSize.y = 0.02f;
    
    TriangleRenderer::PushConstants hlPc = ctx.pushConstants;
    hlPc.color = glm::vec4(1.0f);
    hlPc.model = glm::translate(glm::mat4(1.0f), hlCenter) *
    glm::scale(glm::mat4(1.0f), hlSize);
    vkCmdPushConstants(cmd, triangle.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(TriangleRenderer::PushConstants), &hlPc);
    vkCmdDrawIndexed(cmd, cubeMesh.indexCount(), 1, 0, 0, 0);
}

void SceneRenderer::drawPreview(VkCommandBuffer cmd, const DrawContext& ctx)
{
    if (!ctx.editor.hasPreview()) return;
    if (ctx.editor.activeTool() != SceneEditor::Tool::Place) return;
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle.getPipeline());
    VkDescriptorSet sceneDs = uniformBuffer.descriptorSet(ctx.imageIndex);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            triangle.getPipelineLayout(), 0, 1, &sceneDs, 0, nullptr);
    vkCmdPushConstants(cmd, triangle.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(TriangleRenderer::PushConstants), &ctx.pushConstants);
    VkDeviceSize cubeOffset = 0;
    VkBuffer cubeVb = cubeMesh.vertexBuffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &cubeVb, &cubeOffset);
    vkCmdBindIndexBuffer(cmd, cubeMesh.indexBuffer(), 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdBindIndexBuffer(cmd, cubeMesh.indexBuffer(), 0, VK_INDEX_TYPE_UINT32);
    
    AABB pv = ctx.editor.previewAABB();
    glm::vec3 pvCenter = pv.center();
    glm::vec3 pvSize = pv.size();
    
    TriangleRenderer::PushConstants pvPc = ctx.pushConstants;
    pvPc.color = glm::vec4(1.0f);
    pvPc.model = glm::translate(glm::mat4(1.0f), pvCenter) *
                 glm::scale(glm::mat4(1.0f), pvSize);
    vkCmdPushConstants(cmd, triangle.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(TriangleRenderer::PushConstants), &pvPc);
    vkCmdDrawIndexed(cmd, cubeMesh.indexCount(), 1, 0, 0, 0);
}

void SceneRenderer::drawObjects(VkCommandBuffer cmd, const DrawContext& ctx)
{
    const auto& editorObjects = ctx.editor.getObjects();
    if (editorObjects.empty()) return;
    
    VkPipeline scenePipeline = ctx.wireframe
        ? triangle.getWireframePipeline()
        : triangle.getPipeline();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipeline);
    
    VkDescriptorSet sceneDs = uniformBuffer.descriptorSet(ctx.imageIndex);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             triangle.getPipelineLayout(), 0, 1, &sceneDs, 0, nullptr);
    vkCmdPushConstants(cmd, triangle.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(TriangleRenderer::PushConstants), &ctx.pushConstants);
    VkDeviceSize cubeOffset = 0;
    VkBuffer cubeVb = cubeMesh.vertexBuffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &cubeVb, &cubeOffset);
    vkCmdBindIndexBuffer(cmd, cubeMesh.indexBuffer(), 0, VK_INDEX_TYPE_UINT32);
    
    for (int objIdx = 0; objIdx < static_cast<int>(editorObjects.size()); ++objIdx) {
        const auto& obj = editorObjects[objIdx];
        TriangleRenderer::PushConstants objPc = ctx.pushConstants;
        objPc.color = glm::vec4(obj.color(), 1.0f);
        
        if (obj.type == ShapeType::Box) {
            glm::vec3 center = obj.box.center();
            glm::vec3 sz = obj.box.size();
            objPc.model = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), sz);
            vkCmdPushConstants(cmd, triangle.getPipelineLayout(),
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(TriangleRenderer::PushConstants), &objPc);
            vkCmdDrawIndexed(cmd, cubeMesh.indexCount(), 1, 0, 0, 0);
        } else {
            int ci = objIdx < static_cast<int>(prismCache.size()) ? objIdx : -1;
            if (ci >= 0 && prismCache[ci] != nullptr) {
                objPc.model = glm::mat4(1.0f);
                vkCmdPushConstants(cmd, triangle.getPipelineLayout(),
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0, sizeof(TriangleRenderer::PushConstants), &objPc);
                VkBuffer prismVb = prismCache[ci]->vertexBuffer();
                VkDeviceSize prismOff = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &prismVb, &prismOff);
                vkCmdBindIndexBuffer(cmd, prismCache[ci]->indexBuffer(),
                                     0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, prismCache[ci]->indexCount(), 1, 0, 0, 0);
                
                // Rebind cube mesh for subsequent box draws.
                vkCmdBindVertexBuffers(cmd, 0, 1, &cubeVb, &prismOff);
                vkCmdBindIndexBuffer(cmd, cubeMesh.indexBuffer(),
                                     0, VK_INDEX_TYPE_UINT32);
            }
        }
    }
}

void SceneRenderer::drawSelectionLines(VkCommandBuffer cmd, const DrawContext& ctx) {
    lineBatch.clear();
    
    const auto& editorObjects = ctx.editor.getObjects();
    const auto& multiSel = ctx.editor.selectedSet();
    glm::vec3 red(1.0f, 0.3f, 0.3f);
    glm::vec3 green(0.3f, 1.0f, 0.3f);
    glm::vec3 blue(0.3f, 0.3f, 1.0f);
    glm::vec3 yellow(1.0f, 1.0f, 0.2f);
    
    for (int idx : multiSel) {
        if (idx >= 0 && idx < static_cast<int>(editorObjects.size())) {
            AABB box = editorObjects[idx].boundingAABB();
            lineBatch.addAABBEdges(box.min, box.max, red, green, blue);
        }
    }
    
    int sel = ctx.editor.selectedIndex();
    if (sel >= 0 && sel < static_cast<int>(editorObjects.size())) {
        const AABB box = editorObjects[sel].boundingAABB();
        
        if (ctx.editor.isSlicing()) {
            int ax = ctx.editor.getSliceAxis();
            float sp = ctx.editor.getSlicePosition();
            
            glm::vec3 c0 = box.min, c1 = box.min;
            glm::vec3 c2 = box.max, c3 = box.max;
            c0[ax] = sp; c1[ax] = sp; c2[ax] = sp; c3[ax] = sp;
            
            if (ax == 0) {
                c1.y = box.max.y; c3.z = box.min.z;
            } else if (ax == 1) {
                c1.x = box.max.x; c3.z = box.min.z;
            } else {
                c1.x = box.max.x; c3.y = box.min.y;
            }
            
            lineBatch.addLine(c0, c1, yellow);
            lineBatch.addLine(c1, c2, yellow);
            lineBatch.addLine(c2, c3, yellow);
            lineBatch.addLine(c3, c0, yellow);
        }
        
        if (ctx.editor.isDiagonalSliceReady()) {
            glm::vec3 lineA, lineB;
            if (ctx.editor.getDiagonalSlicePreviewLine(lineA, lineB)) {
                glm::vec3 orange(1.0f, 0.5f, 0.0f);
                lineBatch.addLine(lineA, lineB, orange);
            }
        }
    }
    
    if (ctx.editor.isPasting()) {
        for (const SceneObject& pv : ctx.editor.pastePreviewCubes()){
            glm::vec3 cyan(0.2f, 1.0f, 1.0f);
            AABB bound = pv.boundingAABB();
            lineBatch.addAABBEdges(bound.min, bound.max, cyan, cyan, cyan);
        }
    }
    
    lineBatch.upload();
    
    if (!lineBatch.empty()) {
        UniformBuffer::MVPData lineMvp{};
        lineMvp.view = ctx.activeCamera.viewMatrix();
        lineMvp.projection = ctx.activeCamera.projectionMatrix();
        lineMvp.lightPos = glm::vec4(ctx.lightPos[0], ctx.lightPos[1],
                                     ctx.lightPos[2], 1.0f);
        lineMvp.viewPos = glm::vec4(ctx.activeCamera.position(), 1.0f);
        uniformBuffer.update(ctx.imageIndex, lineMvp);
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          lineRenderer.getPipeline());
        VkDescriptorSet lineDs = uniformBuffer.descriptorSet(ctx.imageIndex);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                lineRenderer.getPipelineLayout(),
                                0, 1, &lineDs, 0, nullptr);
        VkDeviceSize lineOffset = 0;
        VkBuffer lineBuf = lineBatch.buffer();
        vkCmdBindVertexBuffers(cmd, 0, 1, &lineBuf, &lineOffset);
        vkCmdDraw(cmd, lineBatch.vertexCount(), 1, 0, 0);
    }
}


void SceneRenderer::drawGizmo(VkCommandBuffer cmd, const DrawContext& ctx){
    VkExtent2D ext = swapchainBundle.extent();
    const uint32_t GIZMO_SIZE = 120;
    
    VkViewport gizmoViewport{};
    gizmoViewport.x = static_cast<float>(ext.width - GIZMO_SIZE);
    gizmoViewport.y = 0.0f;
    gizmoViewport.width = static_cast<float>(GIZMO_SIZE);
    gizmoViewport.height = static_cast<float>(GIZMO_SIZE);
    gizmoViewport.minDepth = 0.0f;
    gizmoViewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &gizmoViewport);
    
    VkRect2D gizmoScissor{};
    gizmoScissor.offset = { static_cast<int32_t>(ext.width - GIZMO_SIZE), 0 };
    gizmoScissor.extent = { GIZMO_SIZE, GIZMO_SIZE };
    vkCmdSetScissor(cmd, 0, 1, &gizmoScissor);
    
    glm::mat4 rotOnlyView = glm::mat4(glm::mat3(ctx.activeCamera.viewMatrix()));
    rotOnlyView[3][2] = -2.0f;
    glm::mat4 gizmoProj = glm::ortho(-1.2f, 1.2f, 1.2f, -1.2f, 0.1f, 10.0f);
    
    GizmoRenderer::PushConstants pc{};
    pc.view = rotOnlyView;
    pc.projection = gizmoProj;
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gizmo.getPipeline());
    vkCmdPushConstants(cmd, gizmo.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(GizmoRenderer::PushConstants), &pc);
    
    VkViewport fullViewport{};
    fullViewport.width = static_cast<float>(ext.width);
    fullViewport.height = static_cast<float>(ext.height);
    fullViewport.minDepth = 0.0f;
    fullViewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &fullViewport);
    
    VkRect2D fullScissor{};
    fullScissor.extent = ext;
    vkCmdSetScissor(cmd, 0, 1, &fullScissor);
}
