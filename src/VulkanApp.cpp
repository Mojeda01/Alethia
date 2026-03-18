#include "VulkanApp.h"
#include "Vertex.h"
#include <imgui.h>

#define GLM_FORCE_DEPTH_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>

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

VulkanApp::VulkanApp(int width, int height, const char* title)
    : window(width, height, title)
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
    glfwSetWindowUserPointer(window.get(), this);
    glfwSetFramebufferSizeCallback(window.get(), VulkanApp::framebufferResizeCallback);
    glfwSetCursorPosCallback(window.get(), VulkanApp::cursorPosCallback);
    glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window.get(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    auto now = std::chrono::steady_clock::now();
    startTime = now;
    lastFrameTime = now;
    std::cout << "Vulkan fully initialized\n";

 
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
            ImGui::Text("Grid: 1000x1000 units");
            ImGui::Text("Grid spacing: 1m / 10m");
    });

    debugUI.addPanel("Lighting", [this]() {
        ImGui::SliderFloat("Light X", &lightPos[0], -50.0f, 50.0f);
        ImGui::SliderFloat("Light Y", &lightPos[1], 0.0f, 50.0f);
        ImGui::SliderFloat("Light Z", &lightPos[2], -50.0f, 50.0f);
    });

    debugUI.addPanel("Render", [this]() {
            ImGui::Checkbox("Wireframe", &wireframe);
    });

}

void VulkanApp::run() {
    while (!window.shouldClose()) {
        window.pollEvents();

        if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
            continue;
        }

        static bool tabWasPressed = false;
        bool tabPressed = glfwGetKey(window.get(), GLFW_KEY_TAB) == GLFW_PRESS;
        if (tabPressed && !tabWasPressed) {
            uiMode = !uiMode;
            if (uiMode) {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
            }
        }
        tabWasPressed = tabPressed;

        drawFrame();
    }
    vkDeviceWaitIdle(device.get());
}

void VulkanApp::cleanup() {
    // nothing yet.
}

void VulkanApp::framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {
    auto* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->framebufferResized = true;
    }
}

void VulkanApp::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    if (app->uiMode) return;

    if (app->firstMouse) {
        app->lastMouseX = xpos;
        app->lastMouseY = ypos;
        app->firstMouse = false;
        return;
    }

    double xOffset = xpos - app->lastMouseX;
    double yOffset = ypos - app->lastMouseY;
    app->lastMouseX = xpos;
    app->lastMouseY = ypos;

    app->camera.processMouse(xOffset, yOffset);
}

void VulkanApp::drawFrame() {
    VkDevice dev = device.get();

    if (framebufferResized) {
        framebufferResized = false;
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

    if (!uiMode) {
        camera.processKeyboard(window.get(), deltaSeconds);
    }

    imgui.newFrame();
    debugUI.draw();

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
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR || framebufferResized || suboptimal) {
        framebufferResized = false;
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
