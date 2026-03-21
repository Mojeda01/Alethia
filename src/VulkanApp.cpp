#include "VulkanApp.h"
#include "Vertex.h"
#include "GizmoMesh.h"
#include "CubeMesh.h"
#include "Log.h"
#include <imgui.h>

#define GLM_FORCE_DEPTH_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include <cmath>

static std::vector<Vertex> makeGridQuad() {
    const float SIZE = 500.0f;
    Vertex v{};
    v.color[0] = 0.3f; v.color[1] = 0.3f; v.color[2] = 0.3f;
    v.normal[0] = 0.0f; v.normal[1] = 1.0f; v.normal[2] = 0.0f;

    std::vector<Vertex> verts(6, v);
    verts[0].position[0] = -SIZE; verts[0].position[1] = 0.0f; verts[0].position[2] = -SIZE;
    verts[1].position[0] =  SIZE; verts[1].position[1] = 0.0f; verts[1].position[2] = -SIZE;
    verts[2].position[0] =  SIZE; verts[2].position[1] = 0.0f; verts[2].position[2] =  SIZE;
    verts[3].position[0] = -SIZE; verts[3].position[1] = 0.0f; verts[3].position[2] = -SIZE;
    verts[4].position[0] =  SIZE; verts[4].position[1] = 0.0f; verts[4].position[2] =  SIZE;
    verts[5].position[0] = -SIZE; verts[5].position[1] = 0.0f; verts[5].position[2] =  SIZE;
    return verts;
}

static GizmoGeometry gizmoGeometry = makeGizmoMesh();
static CubeGeometry cubeGeometry = makeCube(1.0f);

VulkanApp::VulkanApp(int width, int height, const char* title)
    : window(width, height, title)
    , input(window.get())
    , instance()
    , surface(instance.get(), window.get())
    , device(instance.get(), surface.get())
    , swapchainBundle(  device.physical(),
                        device.get(),
                        surface.get(),
                        (uint32_t)width,
                        (uint32_t)height)
    , uniformBuffer(    device.get(),
                        device.physical(),
                        (uint32_t)swapchainBundle.framebuffers().size())
    , triangle(device.get(), swapchainBundle.renderPass(), uniformBuffer.descriptorSetLayout())
    , grid(device.get(), swapchainBundle.renderPass(), uniformBuffer.descriptorSetLayout())
    , commandPool(  device.get(),
                    device.graphicsQueueFamily(),
                    swapchainBundle.framebuffers().size())
    , gridMesh(     device.get(),
                    device.physical(),
                    commandPool.get(),
                    device.graphicsQueue(),
                    makeGridQuad())
    , gizmo(device.get(), swapchainBundle.renderPass())     
    , gizmoMesh(    device.get(),
                    device.physical(),
                    commandPool.get(),
                    device.graphicsQueue(),
                    gizmoGeometry.vertices,  
                    gizmoGeometry.indices)
    , cubeMesh(     device.get(),
                    device.physical(),
                    commandPool.get(),
                    device.graphicsQueue(),
                    cubeGeometry.vertices,
                    cubeGeometry.indices)
    , devTexture(   device.get(),
                    device.physical(),
                    commandPool.get(),
                    device.graphicsQueue())
    , sync(device.get(), (uint32_t)swapchainBundle.framebuffers().size(),
            (uint32_t)swapchainBundle.framebuffers().size())
    , camera(70.0f, static_cast<float>(width) / static_cast<float>(height), 0.01f, 1000.0f) 
    , imgui(    window.get(),
                instance.get(),
                device.physical(),
                device.get(),
                device.graphicsQueueFamily(),
                device.graphicsQueue(),
                swapchainBundle.renderPass(),
                static_cast<uint32_t>(swapchainBundle.framebuffers().size()))
{
    auto now = std::chrono::steady_clock::now();
    startTime = now;
    lastFrameTime = now;
    std::cout << "Vulkan fully initialized\n";

    Log::init(500);
    Log::info("Alethia engine initialized");
 
    debugUI.addPanel("Performance", [this]() {
        float avg = 0.0f;
        for (int i = 0; i < FRAME_TIME_COUNT; ++i) avg += frameTimes[i];
        avg /= static_cast<float>(FRAME_TIME_COUNT);
        ImGui::Text("FPS: %.0f", avg > 0.0f ? 1000.0f / avg : 0.0f);
        ImGui::Text("Frame: %u", frameIndex);
        ImGui::Text("Frame time: %.2f ms", avg);
        ImGui::PlotLines("##frametime", frameTimes, FRAME_TIME_COUNT,
                            frameTimeIndex, nullptr, 0.0f, 2.0f, ImVec2(0, 40));

    });

    debugUI.addPanel("Camera", [this]() {
            glm::vec3 pos = camera.position();
            ImGui::Text("Position: %.0f, %.0f, %.0f", pos.x, pos.y, pos.z);
            ImGui::Text("Yaw: %.1f  Pitch: %.1f", camera.yaw(), camera.pitch());
    });

    debugUI.addPanel("Scene", [this]() {
        editor.drawUI(); 
    });

    debugUI.addPanel("Lighting", [this]() {
        ImGui::SliderFloat("Light X", &lightPos[0], -50.0f, 50.0f);
        ImGui::SliderFloat("Light Y", &lightPos[1], 0.0f, 50.0f);
        ImGui::SliderFloat("Light Z", &lightPos[2], -50.0f, 50.0f);
    });

    debugUI.addPanel("Render", [this]() {
            ImGui::Checkbox("Wireframe", &wireframe);
    });
    
    uniformBuffer.bindTexture(devTexture.view(), devTexture.sampler());
}

void VulkanApp::run() {
    while (!window.shouldClose()) {
        window.pollEvents();

        if (input.isKeyPressed(GLFW_KEY_ESCAPE)){ 
            glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
            continue;
        }

        editor.update(input, camera, window.get());

        input.update();
        drawFrame();
    }
    vkDeviceWaitIdle(device.get());
}

void VulkanApp::cleanup() {
    // nothing yet.
}


glm::vec3 VulkanApp::raycastGrid(double mouseX, double mouseY) const {
    int winW, winH;
    glfwGetWindowSize(window.get(), &winW, &winH);

    float ndcX = (2.0f * static_cast<float>(mouseX)) / static_cast<float>(winW) - 1.0f;
    float ndcY = 1.0f - (2.0f * static_cast<float>(mouseY)) / static_cast<float>(winH);

    glm::vec4 clipNear(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 clipFar(ndcX, ndcY, 1.0f, 1.0f);

    glm::mat4 proj = camera.projectionMatrix();
    proj[1][1] *= -1.0f;
    glm::mat4 invProj = glm::inverse(proj);
    glm::mat4 invView = glm::inverse(camera.viewMatrix());

    glm::vec4 viewNear = invProj * clipNear;
    viewNear /= viewNear.w;
    glm::vec4 viewFar = invProj * clipFar;
    viewFar /= viewFar.w;

    glm::vec3 worldNear = glm::vec3(invView * viewNear);
    glm::vec3 worldFar = glm::vec3(invView * viewFar);

    glm::vec3 rayDir = glm::normalize(worldFar - worldNear);
    glm::vec3 rayOrigin = worldNear;

    if (std::abs(rayDir.y) < 0.0001f) {
        return glm::vec3(0.0f, -10000.0f, 0.0f);
    }

    float t = -rayOrigin.y / rayDir.y;
    if (t < 0.0f) {
        return glm::vec3(0.0f, -10000.0f, 0.0f);
    }
    return rayOrigin + rayDir * t;
}

void VulkanApp::drawFrame(){
    VkDevice dev = device.get();

    if (input.framebufferWasResized()) {
        input.clearFramebufferResized();
        recreateSwapchain();
        return;
    }

    // wait for the CPU/GPU sync fence for this frame
    VkFence frameFence = sync.inFlightFence();
    vkWaitForFences(dev, 1, &frameFence, VK_TRUE, UINT64_MAX);

    // get an image from swapchain.
    uint32_t imageIndex = 0;
    VkResult r = vkAcquireNextImageKHR(
        dev,
        swapchainBundle.swapchain(),
        UINT64_MAX,
        sync.imageAvailable(),
        VK_NULL_HANDLE,
        &imageIndex
    );
    bool suboptimal = (r == VK_SUBOPTIMAL_KHR);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } 
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("vkAcquireNextImageKHR failed");
    }

    // If that swapchain is already in flight, wait for it.
    sync.waitForImage(imageIndex);
    sync.markImageInFlight(imageIndex);

    // reset fence now fresh work for this frame is coming
    vkResetFences(dev, 1, &frameFence);

    // record clear-only render pass into the command buffer.
    VkCommandBuffer cmd = commandPool.buffers()[imageIndex];
    vkResetCommandBuffer(cmd, 0);
    
    auto now = std::chrono::steady_clock::now();
    float timeSeconds = std::chrono::duration<float>(now - startTime).count();
    float deltaSeconds = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;
    
    frameTimes[frameTimeIndex] = deltaSeconds * 1000.0f;
    frameTimeIndex = (frameTimeIndex + 1) % FRAME_TIME_COUNT;

    if (!input.inUIMode()) {
        camera.processKeyboard(window.get(), deltaSeconds);
        camera.processMouse(input.mouseDeltaX(), input.mouseDeltaY());
    }

    imgui.newFrame();
    debugUI.draw();

    ImGui::Begin("Log");
    Log::drawPanel();
    ImGui::End();

    UniformBuffer::MVPData mvp{};
    mvp.model = glm::mat4(1.0f);
    mvp.view = camera.viewMatrix();
    mvp.projection = camera.projectionMatrix();
    mvp.lightPos = glm::vec4(lightPos[0], lightPos[1], lightPos[2], 1.0f);
    mvp.viewPos = glm::vec4(camera.position(), 1.0f);
    uniformBuffer.update(imageIndex, mvp);

    TriangleRenderer::PushConstants pushConstants{};
    pushConstants.timeSeconds = timeSeconds;
    pushConstants.deltaSeconds = deltaSeconds;
    pushConstants.frameIndex = frameIndex++;
    Log::setFrame(frameIndex);

    recordCommandBuffer(cmd, imageIndex, pushConstants);

    // submit
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSemaphore waitSem = sync.imageAvailable();
    VkSemaphore signalSem = sync.renderFinished();

    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &waitSem;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &signalSem;

    if (vkQueueSubmit(device.graphicsQueue(), 1, &submit, frameFence) != VK_SUCCESS) {
        throw std::runtime_error("vkQueueSubmit failed");
    }

    // Present
    VkSwapchainKHR sc = swapchainBundle.swapchain();
    VkPresentInfoKHR present{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &signalSem;
    present.swapchainCount = 1;
    present.pSwapchains = &sc;
    present.pImageIndices = &imageIndex;

    r = vkQueuePresentKHR(device.presentQueue(), &present);
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR || input.framebufferWasResized() || suboptimal){
        input.clearFramebufferResized();
        recreateSwapchain();
        return;
    } else if (r != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    sync.advanceFrame();
}

void VulkanApp::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, const TriangleRenderer::PushConstants& pushConstants) {
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { { 0.12f, 0.12f, 0.14f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rp{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rp.renderPass = swapchainBundle.renderPass();
    rp.framebuffer = swapchainBundle.framebuffers()[imageIndex];
    rp.renderArea.offset = { 0, 0 };
    rp.renderArea.extent = swapchainBundle.extent();
    rp.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rp.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.width = static_cast<float>(swapchainBundle.extent().width);
    viewport.height = static_cast<float>(swapchainBundle.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = swapchainBundle.extent();
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, grid.getPipeline());

    VkDescriptorSet ds = uniformBuffer.descriptorSet(imageIndex);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, grid.getPipelineLayout(), 
                            0, 1, &ds, 0, nullptr);

    VkDeviceSize offset = 0;
    VkBuffer gridVb = gridMesh.vertexBuffer();
    vkCmdBindVertexBuffers(cmd, 0, 1, &gridVb, &offset);
    vkCmdDraw(cmd, 6, 1, 0, 0);
    
    const auto& editorCubes = editor.getCubes();

    if (editor.hasHighlight()) {
        VkPipeline scenePipeline = triangle.getPipeline();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipeline);

        VkDescriptorSet sceneDs = uniformBuffer.descriptorSet(imageIndex);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle.getPipelineLayout(),
                                    0, 1, &sceneDs, 0, nullptr);
        vkCmdPushConstants(cmd, triangle.getPipelineLayout(),
                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                0, sizeof(TriangleRenderer::PushConstants), &pushConstants);
        VkDeviceSize cubeOffset = 0;
        VkBuffer cubeVb = cubeMesh.vertexBuffer();
        vkCmdBindVertexBuffers(cmd, 0, 1, &cubeVb, &cubeOffset);
        VkBuffer cubeIb = cubeMesh.indexBuffer();
        vkCmdBindIndexBuffer(cmd, cubeIb, 0, VK_INDEX_TYPE_UINT32);

        glm::vec3 hlMin = editor.highlightMin();
        glm::vec3 hlMax = editor.highlightMax();
        glm::vec3 hlCenter = (hlMin + hlMax) * 0.5f;
        glm::vec3 hlSize = hlMax - hlMin;
        if (hlSize.y < 0.02f) hlSize.y = 0.02f;

        UniformBuffer::MVPData hlMvp{};
        hlMvp.model = glm::translate(glm::mat4(1.0f), hlCenter) *
                        glm::scale(glm::mat4(1.0f), hlSize);
        hlMvp.view = camera.viewMatrix();
        hlMvp.projection = camera.projectionMatrix();
        hlMvp.lightPos = glm::vec4(lightPos[0], lightPos[1], lightPos[2], 1.0f);
        hlMvp.viewPos = glm::vec4(camera.position(), 1.0f);
        uniformBuffer.update(imageIndex, hlMvp);
        vkCmdDrawIndexed(cmd, cubeMesh.indexCount(), 1, 0, 0, 0);
    }

    if (editor.hasPreview()) { 
        VkPipeline scenePipeline = triangle.getPipeline();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipeline);

        VkDescriptorSet sceneDs = uniformBuffer.descriptorSet(imageIndex);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle.getPipelineLayout(),
                                0, 1, &sceneDs, 0, nullptr);
        vkCmdPushConstants(cmd, triangle.getPipelineLayout(),
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                             0, sizeof(TriangleRenderer::PushConstants), &pushConstants);
        VkDeviceSize cubeOffset = 0;
        VkBuffer cubeVb = cubeMesh.vertexBuffer();
        vkCmdBindVertexBuffers(cmd, 0, 1, &cubeVb, &cubeOffset);
        VkBuffer cubeIb = cubeMesh.indexBuffer();
        vkCmdBindIndexBuffer(cmd, cubeIb, 0, VK_INDEX_TYPE_UINT32);

        AABB pv = editor.previewAABB();
        glm::vec3 pvCenter = pv.center();
        glm::vec3 pvSize = pv.size();
        UniformBuffer::MVPData pvMvp{};
        pvMvp.model = glm::translate(glm::mat4(1.0f), pvCenter) *
                        glm::scale(glm::mat4(1.0f), pvSize);
        pvMvp.view = camera.viewMatrix();
        pvMvp.projection = camera.projectionMatrix();
        pvMvp.lightPos = glm::vec4(lightPos[0], lightPos[1], lightPos[2], 1.0f);
        pvMvp.viewPos = glm::vec4(camera.position(), 1.0f);
        uniformBuffer.update(imageIndex, pvMvp);
        vkCmdDrawIndexed(cmd, cubeMesh.indexCount(), 1, 0, 0, 0);
    }

    // Draw Cubes
    if (!editorCubes.empty()) {
        VkPipeline scenePipeline = wireframe ? triangle.getWireframePipeline() : triangle.getPipeline();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipeline);

        VkDescriptorSet sceneDs = uniformBuffer.descriptorSet(imageIndex);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle.getPipelineLayout(),
                                    0, 1, &sceneDs, 0, nullptr);

        vkCmdPushConstants(cmd, triangle.getPipelineLayout(),
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                            0, sizeof(TriangleRenderer::PushConstants), &pushConstants);

        VkDeviceSize cubeOffset = 0;
        VkBuffer cubeVb = cubeMesh.vertexBuffer();
        vkCmdBindVertexBuffers(cmd, 0, 1, &cubeVb, &cubeOffset);
        VkBuffer cubeIb = cubeMesh.indexBuffer();
        vkCmdBindIndexBuffer(cmd, cubeIb, 0, VK_INDEX_TYPE_UINT32);

        
       
     for (int i = 0; i < static_cast<int>(editorCubes.size()); ++i) {
            const AABB& cube = editorCubes[i];
            glm::vec3 center = cube.center();
            glm::vec3 sz = cube.size();
            UniformBuffer::MVPData cubeMvp{};
            cubeMvp.model = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), sz); 
            cubeMvp.view = camera.viewMatrix();
            cubeMvp.projection = camera.projectionMatrix();
            cubeMvp.lightPos = glm::vec4(lightPos[0], lightPos[1], lightPos[2], 1.0f);
            cubeMvp.viewPos = glm::vec4(camera.position(), 1.0f);
            uniformBuffer.update(imageIndex, cubeMvp);
            vkCmdDrawIndexed(cmd, cubeMesh.indexCount(), 1, 0, 0, 0);

            if (i == editor.selectedIndex()) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle.getWireframePipeline());
                vkCmdDrawIndexed(cmd, cubeMesh.indexCount(), 1, 0, 0, 0);
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipeline);
            }
        }
    }
    
    // Gizmo code
    {
        VkExtent2D ext = swapchainBundle.extent();
        const uint32_t GIZMO_SIZE = 120;

        VkViewport gizmoViewport{};
        gizmoViewport.x      = static_cast<float>(ext.width - GIZMO_SIZE);
        gizmoViewport.y      = 0.0f;
        gizmoViewport.width  = static_cast<float>(GIZMO_SIZE);
        gizmoViewport.height = static_cast<float>(GIZMO_SIZE);
        gizmoViewport.minDepth = 0.0f;
        gizmoViewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &gizmoViewport);

        VkRect2D gizmoScissor{};
        gizmoScissor.offset = { static_cast<int32_t>(ext.width - GIZMO_SIZE), 0 };
        gizmoScissor.extent = { GIZMO_SIZE, GIZMO_SIZE };
        vkCmdSetScissor(cmd, 0, 1, &gizmoScissor); 
        
        glm::mat4 rotOnlyView = glm::mat4(glm::mat3(camera.viewMatrix()));
        rotOnlyView[3][2] = -2.0f;
        glm::mat4 gizmoProj = glm::ortho(-1.2f, 1.2f, 1.2f, -1.2f, 0.1f, 10.0f);

        GizmoRenderer::PushConstants pc{};
        pc.view       = rotOnlyView;
        pc.projection = gizmoProj;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gizmo.getPipeline());
        vkCmdPushConstants(cmd, gizmo.getPipelineLayout(),
                            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GizmoRenderer::PushConstants), &pc);

        VkBuffer gizmoVb = gizmoMesh.vertexBuffer();
        vkCmdBindVertexBuffers(cmd, 0, 1, &gizmoVb, &offset);
        VkBuffer gizmoIb = gizmoMesh.indexBuffer();
        vkCmdBindIndexBuffer(cmd, gizmoIb, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, gizmoMesh.indexCount(), 1, 0, 0, 0);
        
        VkViewport fullViewport{};
        fullViewport.width    = static_cast<float>(ext.width);
        fullViewport.height   = static_cast<float>(ext.height);
        fullViewport.minDepth = 0.0f;
        fullViewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &fullViewport);

        VkRect2D fullScissor{};
        fullScissor.extent = ext;
        vkCmdSetScissor(cmd, 0, 1, &fullScissor);
    }

    imgui.render(cmd);
    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
}

void VulkanApp::recreateSwapchain() {
    auto fb = window.framebufferSize();
    while (fb.first == 0 || fb.second == 0) {
        window.pollEvents();
        fb = window.framebufferSize();
    }

    vkDeviceWaitIdle(device.get());

    swapchainBundle.recreate(
        device.physical(),
        device.get(),
        surface.get(),
        static_cast<uint32_t>(fb.first),
        static_cast<uint32_t>(fb.second)
    );

    camera.setAspectRatio(static_cast<float>(fb.first) / static_cast<float>(fb.second));
}
