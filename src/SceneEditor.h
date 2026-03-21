#pragma once

#include "AABB.h"
#include "InputManager.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

class SceneEditor {
public:
    enum class Tool : uint8_t {
        Place,
        Select,
        Slice
    };

    SceneEditor() = default;

    void update(const InputManager& input, const Camera& camera, GLFWwindow* window);
    void drawUI();

    const std::vector<AABB>& getCubes() const { return cubes; }
    int selectedIndex() const { return selected; }

    bool hasHighlight() const { return highlightValid; }
    glm::vec3 highlightMin() const { return highlightCellMin; }
    glm::vec3 highlightMax() const { return highlightCellMax; }

    bool hasPreview() const { return dragging; }
    AABB previewAABB() const { return preview; }
    float getGridSnap() const { return gridSnap; }
    Tool activeTool() const { return tool; }

    bool isDraggingFace() const { return dragFace >= 0; }
    int activeDragFace() const { return dragFace; }
 
    int getSliceAxis() const { return sliceAxis; }
    float getSlicePosition() const { return slicePosition; }
    bool isSlicing() const { return sliceActive && selected >= 0; } 

private:
    glm::vec3 raycastGrid(const InputManager& input, const Camera& camera, GLFWwindow* window) const;
    glm::vec3 worldRayDir(const InputManager& input, const Camera& camera, GLFWwindow* window) const;
    glm::vec3 worldRayOrigin(const Camera& camera) const;
    int hitTestCube(const glm::vec3& rayOrigin, const glm::vec3& rayDir, int cubeIndex, float& outT) const;
    int hitTestFace(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& outT) const;
    float snapValue(float val) const;

    std::vector<AABB> cubes;
    int selected = -1;
    Tool tool = Tool::Place;
    float gridSnap = 1.0f;

    bool highlightValid = false;
    glm::vec3 highlightCellMin{0.0f};
    glm::vec3 highlightCellMax{0.0f};

    bool dragging = false;
    glm::vec3 dragStart{0.0f};
    AABB preview{};

    int dragFace = -1;

    int sliceAxis = -1;
    bool sliceActive = false;
    float slicePosition = 0.0f;
};
