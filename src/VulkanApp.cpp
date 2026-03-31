#include "VulkanApp.h"
#include "Vertex.h"
#include "GizmoMesh.h"
#include "CubeMesh.h"
#include "Log.h"
#include "SceneEditor.h"
#include "SceneObject.h"
#include "TriangularPrismMesh.h"
#include <imgui.h>

#define GLM_FORCE_DEPTH_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    , lineRenderer(device.get(), swapchainBundle.renderPass(), uniformBuffer.descriptorSetLayout())
    , commandPool(  device.get(),
                    device.graphicsQueueFamily(),
                    swapchainBundle.framebuffers().size())
    , meshRenderer(device,
                   commandPool,
                   device.graphicsQueue(),
                   swapchainBundle.renderPass(),
                   uniformBuffer.descriptorSetLayout())
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
    , lineBatch(    device.get(),
                    device.physical(),
                    commandPool.get(),
                    device.graphicsQueue())
    , sync(device.get(), (uint32_t)swapchainBundle.framebuffers().size(),
            (uint32_t)swapchainBundle.framebuffers().size())
    , camera(70.0f, static_cast<float>(width) / static_cast<float>(height), 0.01f, 1000.0f)
    , playerCamera(70.0f, static_cast<float>(width) / static_cast<float>(height), 0.01f, 1000.0f)
    , imgui(    window.get(),
                instance.get(),
                device.physical(),
                device.get(),
                device.graphicsQueueFamily(),
                device.graphicsQueue(),
                swapchainBundle.renderPass(),
                static_cast<uint32_t>(swapchainBundle.framebuffers().size()))
    , objModelMenu(meshRenderer)
    , sceneRenderer(
                    device.get(), device.physical(),
                    commandPool.get(), device.graphicsQueue(),
                    swapchainBundle, uniformBuffer,
                    triangle, grid, lineRenderer,
                    gizmo, cubeMesh, gridMesh, gizmoMesh,
                    lineBatch, imgui, meshRenderer)
{
    auto now = std::chrono::steady_clock::now();
    startTime = now;
    lastFrameTime = now;  
    
    input.installCallbacks();

    Log::init(500);
    Log::info("Alethia engine initialized");
    
    // The External tool execution panels.
    externalTools.addPanel("Test Tool", [this]() {
        if (ImGui::Button("Run TestBinaryTool")) {
            externalTools.testExecution();
        }
    });
    
    // The debug panel section
    debugUI.addPanel("General", [this]() {
        ImGui::InputText("File", sceneFilename.data(), sceneFilename.capacity() + 1); 
        if (ImGui::Button("New")) {
            editor.newProject();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            editor.saveToFile(std::string(sceneFilename));
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            editor.loadFromFile(sceneFilename); 
        }
        if (ImGui::Button("Quit")) {
            glfwSetWindowShouldClose(window.get(), GLFW_TRUE); 
        } 
    });

    debugUI.addPanel("Performance", [this]() {
        float avg = 0.0f;
        for (int i = 0; i < FRAME_TIME_COUNT; ++i) avg += frameTimes[i];
        avg /= static_cast<float>(FRAME_TIME_COUNT);
        ImGui::Text("FPS: %.0f", avg > 0.0f ? 1000.0f / avg : 0.0f);
        ImGui::Text("Frame: %u", frameIndex);
        ImGui::Text("Frame time: %.2f ms", avg);
        ImGui::PlotLines("##frametime", frameTimes.data(), FRAME_TIME_COUNT,
                            frameTimeIndex, nullptr, 0.0f, 2.0f, ImVec2(0, 40));

    });

    debugUI.addPanel("Camera", [this]() {
        if (appMode == AppMode::Edit) {
            glm::vec3 pos = camera.position();
            ImGui::Text("Mode: Edit");
            ImGui::Text("Position: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z);
            ImGui::Text("Yaw: %.1f  Pitch: %.1f", camera.yaw(), camera.pitch());
        } else {
            const auto& body = player.body();
            ImGui::Text("Mode: Play");
            ImGui::Text("Position: %.1f, %.1f, %.1f",
                    body.position.x, body.position.y, body.position.z);
            ImGui::Text("Velocity: %.1f, %.1f, %.1f",
                    body.velocity.x, body.velocity.y, body.velocity.z);
            ImGui::Text("OnGround: %s  Noclip: %s",
                    body.onGround ? "yes" : "no",
                    body.noclip   ? "yes" : "no");
            ImGui::Text("[P] Edit mode  [F] Noclip  [Space] Jump");
        }     
    }); 

    debugUI.addPanel("Scene", [this]() {
        editor.drawUI(); 
    });

    debugUI.addPanel("Lighting", [this]() {
        ImGui::SliderFloat("Light X", &lightPos.x, -50.0f, 50.0f);
        ImGui::SliderFloat("Light Y", &lightPos.y, 0.0f, 50.0f);
        ImGui::SliderFloat("Light Z", &lightPos.z, -50.0f, 50.0f);
    });

    debugUI.addPanel("Render", [this]() {
            ImGui::Checkbox("Wireframe", &wireframe);        
    });

    uniformBuffer.bindTexture(devTexture.view(), devTexture.sampler());
}

void VulkanApp::run() {
    std::chrono::steady_clock::time_point lastRunTime = startTime;

    while (!window.shouldClose()) {
        window.pollEvents();   
        
        auto nowRun = std::chrono::steady_clock::now();
        float deltaSeconds = std::chrono::duration<float>(nowRun - lastRunTime).count();
        lastRunTime = nowRun;
        deltaSeconds = std::min(deltaSeconds, 0.05f); // cap at 50ms to prevent tunnelling on lag spike

        // P key toggles play/edit mode
        if (input.wasKeyJustPressed(GLFW_KEY_P)) {
            if (appMode == AppMode::Edit) {
                appMode = AppMode::Play;
                // spawn player at editor camera position
                glm::vec3 spawnPos = camera.position();
                spawnPos.y -= player.cfg.eyeHeight;
                if (spawnPos.y < 0.0f) spawnPos.y = 0.0f;
                player.setPosition(spawnPos);
                input.setUIMode(false); // lock cursor for play mode
                Log::info("Entered Play mode — F to toggle noclip, P to return");
            } else {
                appMode = AppMode::Edit;
                // return editor camera to player eye position
                camera.setPosition(player.eyePosition());
                camera.setOrientation(player.yaw(), player.pitch());
                input.setUIMode(true); // release cursor for edit mode
                Log::info("Returned to Edit mode");
            }
        }

        if (input.wasKeyJustPressed(GLFW_KEY_M) &&
                !input.imguiWantsKeyboard() &&
                appMode == AppMode::Edit) {
            materialPanel.visible = !materialPanel.visible;
        }
        static uint64_t lastSceneVersion = 0;
        if (appMode == AppMode::Edit) {
            editor.update(input, camera, window.get());
            if (editor.sceneVersion() != lastSceneVersion) {
                sceneRenderer.markDirty();
                lastSceneVersion = editor.sceneVersion();
            }
        } else {
            player.update(input, physicsSolver, editor.getObjects(), deltaSeconds);
            playerCamera.setPosition(player.eyePosition());
            playerCamera.setOrientation(player.yaw(), player.pitch());
        }
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
    Camera& activeCamera = (appMode == AppMode::Play) ? playerCamera : camera;

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

    if (appMode == AppMode::Edit && !input.inUIMode()) {
        camera.processKeyboard(window.get(), deltaSeconds);  
        camera.processMouse(input.mouseDeltaX(), input.mouseDeltaY());
    }

    imgui.newFrame();
    debugUI.draw();
    externalTools.draw();
    materialPanel.draw();
    
    if (objModelMenu.isVisible()) {
        objModelMenu.draw();
    }

    if (materialPanel.colorWasJustSelected() && appMode == AppMode::Edit) {
        editor.applyColorToSelection(materialPanel.selectedColor());
        editor.setActiveColor(materialPanel.selectedColor());
        materialPanel.clearPendingSelection();
    }

    ImGui::Begin("Log");
    Log::drawPanel();
    ImGui::End();

    UniformBuffer::MVPData mvp{};
    mvp.view = activeCamera.viewMatrix(); 
    mvp.projection = activeCamera.projectionMatrix(); 
    mvp.lightPos = glm::vec4(lightPos, 1.0f); // // glm::vec3 converts cleanly to vec4 
    mvp.viewPos = glm::vec4(activeCamera.position(), 1.0f); 
    uniformBuffer.update(imageIndex, mvp);

    TriangleRenderer::PushConstants pushConstants{};
    pushConstants.timeSeconds = timeSeconds;
    pushConstants.deltaSeconds = deltaSeconds;
    pushConstants.frameIndex = frameIndex++;
    Log::setFrame(frameIndex);
    
    sceneRenderer.rebuildPrismCacheIfNeeded(editor.getObjects());
    SceneRenderer::DrawContext ctx{
        imageIndex,
        activeCamera,
        pushConstants,
        editor,
        wireframe,
        glm::value_ptr(lightPos) // converts glm::vec3 to const float*
    };
    sceneRenderer.record(cmd, ctx);

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
    playerCamera.setAspectRatio(static_cast<float>(fb.first) / static_cast<float>(fb.second));
}
