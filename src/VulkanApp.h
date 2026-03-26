#pragma once

#include "Window.h"
#include "InputManager.h"
#include "Instance.h"
#include "Surface.h"
#include "Device.h"
#include "Swapchain.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "CommandPool.h"
#include "SwapchainBundle.h"
#include "FrameSync.h"
#include "MeshBuffer.h"
#include "UniformBuffer.h"
#include "Camera.h"
#include "TextureImage.h"
#include "ImGuiLayer.h"
#include "DebugUI.h"
#include "GridRenderer.h"
#include "LineRenderer.h"
#include "LineBatch.h"
#include "triangle/TriangleRenderer.h"
#include "GizmoRenderer.h"
#include "GizmoMesh.h"
#include "CubeMesh.h"
#include "DevTexture.h"
#include "AABB.h"
#include "SceneEditor.h"
#include "AppMode.h"
#include "PlayerController.h"
#include "PhysicsSolver.h"
#include "MaterialPanel.h"
#include "SceneRenderer.h"

#include <chrono>
#include <cstdint>
#include <vector>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

class VulkanApp{
private:
    AppMode appMode = AppMode::Edit;
    PlayerController player;
    PhysicsSolver physicsSolver;
    Camera playerCamera;
public:
    VulkanApp(int width, int height, const char* title);
    VulkanApp(const VulkanApp&) = delete;
    VulkanApp& operator=(const VulkanApp&) = delete;
    void run();
private:
    void cleanup();
    void drawFrame();
 
    void recreateSwapchain(); 
    glm::vec3 raycastGrid(double mouseX, double mouseY) const;
private:
    Window window;
    InputManager input;
    Instance instance;
    Surface surface;
    Device device;
    SwapchainBundle swapchainBundle; 
    UniformBuffer uniformBuffer;
    TriangleRenderer triangle;
    GridRenderer grid;
    LineRenderer lineRenderer;
    CommandPool commandPool;
    MeshBuffer gridMesh;
    GizmoRenderer gizmo;
    MeshBuffer gizmoMesh;
    MeshBuffer cubeMesh;
    DevTexture devTexture;
    LineBatch lineBatch;
    SceneEditor editor; 
    FrameSync sync;
    Camera camera;
    ImGuiLayer imgui;
    DebugUI debugUI;
    MaterialPanel materialPanel;
    SceneRenderer sceneRenderer;
    glm::vec3 lightPos { 5.0f, 10.0f, 5.0f };
    std::string sceneFilename = "scene.alethia";
    bool wireframe = false;
    static constexpr int FRAME_TIME_COUNT = 120;
    std::array<float, FRAME_TIME_COUNT> frameTimes{}; 
    int frameTimeIndex = 0;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastFrameTime;
    std::uint32_t frameIndex = 0; 
};
