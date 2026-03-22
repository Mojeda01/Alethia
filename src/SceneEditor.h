#pragma once

#include "AABB.h"
#include "InputManager.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <deque>

class SceneEditor {
public:
    enum class Tool : uint8_t {
        Place,
        Select,
        Slice,
        Move
    };

    SceneEditor() { newProject(); } 

    void update(const InputManager& input, const Camera& camera, GLFWwindow* window);
    void drawUI();
    
    void newProject();
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

    const std::vector<AABB>& getCubes() const { return cubes; }
    int selectedIndex() const { return selected; }
    const std::unordered_set<int>& selectedSet() const { return multiSelected; }
    bool isMultiSelected(int i) const { return multiSelected.count(i) > 0; }
    bool hasClipboard() const { return !clipboard.empty(); }

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

    bool isPasting() const { return pasting; }
    const std::vector<AABB>& pastePreviewCubes() const { return pastePreview; }

    void undo();
    void redo();
    bool canUndo() const { return historyCursor > 0; }
    bool canRedo() const { return historyCursor < static_cast<int>(history.size()) - 1; }

private:
    void pushSnapshot();
    void clearSelection();
    void buildPastePreview(float targetX, float targetY, float targetZ);
    void eraseAndRemap(int index);
    int hitTestSurface(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& outY) const; 

    std::deque<std::vector<AABB>> history;
    int historyCursor = -1;
    static constexpr int MAX_HISTORY = 64;

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

    float placementY = 0.0f;
    int surfaceHitCube = -1;

    int dragFace = -1;

    int sliceAxis = -1;
    bool sliceActive = false;
    float slicePosition = 0.0f;

    bool moving = false;
    glm::vec3 moveOffset{0.0f};

    // multi-select and clipboard
    std::unordered_set<int> multiSelected;
    std::vector<AABB> clipboard;
    glm::vec3 clipboardOrigin{0.0f};

    // paste preview
    bool pasting = false;
    std::vector<AABB> pastePreview;  
    
};
