#pragma once

#include "AABB.h"
#include "SceneObject.h"
#include "SceneEditor.h"
#include "Camera.h"
#include "MeshBuffer.h"
#include "LineBatch.h"
#include "UniformBuffer.h"
#include "ImGuiLayer.h"
#include "SwapchainBundle.h"
#include "triangle/TriangleRenderer.h"
#include "GridRenderer.h"
#include "LineRenderer.h"
#include "GizmoRenderer.h"
#include "TriangularPrismMesh.h"
#include "MeshRenderer.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class SceneRenderer{
public:
    struct DrawContext{
        uint32_t imageIndex;
        const Camera& activeCamera;
        const TriangleRenderer::PushConstants& pushConstants;
        const SceneEditor& editor;
        bool wireframe;
        const float* lightPos;
    };
    
    SceneRenderer(
                  VkDevice device,
                  VkPhysicalDevice physical,
                  VkCommandPool commandPool,
                  VkQueue graphicsQueue,
                  SwapchainBundle& swapchainBundle,
                  UniformBuffer& uniformBuffer,
                  TriangleRenderer& triangle,
                  GridRenderer& grid,
                  LineRenderer& lineRenderer,
                  GizmoRenderer& gizmo,
                  MeshBuffer& cubeMesh,
                  MeshBuffer& gridMesh,
                  MeshBuffer& gizmoMesh,
                  LineBatch& lineBatch,
                  ImGuiLayer& imgui,
                  MeshRenderer& meshRenderer
    );
    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;
    
    void record(VkCommandBuffer cmd, const DrawContext& ctx);
    void markDirty() { prismCacheDirty = true; }
    void rebuildPrismCacheIfNeeded(const std::vector<SceneObject>& objects);
private:
    void rebuildPrismCache(const std::vector<SceneObject>& objects);
    
    void drawGrid(VkCommandBuffer cmd, uint32_t imageIndex);
    void drawHighlight(VkCommandBuffer cmd, const DrawContext& ctx);
    void drawPreview(VkCommandBuffer cmd, const DrawContext& ctx);
    void drawObjects(VkCommandBuffer cmd, const DrawContext& ctx);
    void drawSelectionLines(VkCommandBuffer cmd, const DrawContext& ctx);
    void drawGizmo(VkCommandBuffer cmd, const DrawContext& ctx);
    
    // Borrow - not owned
    
    VkDevice device;
    VkPhysicalDevice physical;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;
    SwapchainBundle& swapchainBundle;
    UniformBuffer& uniformBuffer;
    TriangleRenderer& triangle;
    GridRenderer& grid;
    LineRenderer& lineRenderer;
    GizmoRenderer& gizmo;
    MeshBuffer& cubeMesh;
    MeshBuffer& gridMesh;
    MeshBuffer& gizmoMesh;
    LineBatch& lineBatch;
    ImGuiLayer& imgui;
    MeshRenderer& meshRenderer;
    
    // owned
    std::vector<std::unique_ptr<MeshBuffer>> prismCache;
    bool prismCacheDirty = true;
};
