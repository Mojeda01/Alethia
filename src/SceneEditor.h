#pragma once

#include "AABB.h"
#include "SceneObject.h"
#include "TriangularPrism.h"
#include "InputManager.h"
#include "SpatialIndex.h"
#include "Camera.h"
#include "Mesh.h"

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
    
    enum class SliceMode : uint8_t {
        Axis, // existing axis-aligned slice
        DiagonalXZ, // diagonal in XZ plane
        DiagonalXY, // diagonal in XY plane
        DiagonalYZ  // diagonal in YZ plane
    };

    SceneEditor() { newProject(); }  

    void update(const InputManager& input, const Camera& camera, GLFWwindow* window);
    void drawUI();
    
    void newProject();
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

    const std::vector<SceneObject>& getObjects() const { return objects; }
    
    // Add a loaded mesh to the scene so it is managed by SceneEditor
    void addMesh(const Mesh& meshData, const glm::vec3& position = glm::vec3(0.0f, 2.0f, 0.0f));
    
    // backward-compatible helper - returns bounding  AABBs for physics.
    std::vector<AABB> getCollisionAABBs() const {
        std::vector<AABB> result;
        result.reserve(objects.size());
        for (const auto& obj : objects) {
            result.push_back(obj.boundingAABB());
        }
        return result;
    }
    
    int selectedIndex() const { return selected; }
    const std::unordered_set<int>& selectedSet() const { return multiSelected; }
    bool isMultiSelected(int i) const { return multiSelected.count(i) > 0; }
    bool hasClipboard() const { return !clipBoard.empty(); }

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
    bool isDiagonalSliceReady() const {
        return tool == Tool::Slice && sliceMode != SliceMode::Axis && selected >= 0;
    }
    // return the two endpoints of the diagonal cut line for preview rendering
    bool getDiagonalSlicePreviewLine(glm::vec3& outA, glm::vec3& outB) const;
    
    // executes the diagonal slice - called by ImGui button
    void applyDiagonalSlice();
    SliceMode getSliceMode() const { return sliceMode; }

    bool isPasting() const { return pasting; }
    const std::vector<SceneObject>& pastePreviewCubes() const { return pastePreview; }

    void undo();
    void redo();
    bool canUndo() const { return historyCursor > 0; }
    bool canRedo() const { return historyCursor < static_cast<int>(history.size()) - 1; }

    void applyColorToSelection(const glm::vec3& color);
    void setActiveColor(const glm::vec3& color) { activeColor = color; }
    
    uint64_t sceneVersion() const { return sceneVer; }
    
private:
    void pushSnapshot();
    uint64_t sceneVer = 0;
    void clearSelection();
    void buildPastePreview(float targetX, float targetY, float targetZ);
    void eraseAndRemap(int index);
    int hitTestSurface(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& outY) const; 

    SceneObject createMeshSceneObject(const Mesh& meshData, const glm::vec3& position);
    
    std::deque<std::vector<SceneObject>> history;
    int historyCursor = -1;
    static constexpr int MAX_HISTORY = 64;

    glm::vec3 raycastGrid(const InputManager& input, const Camera& camera, GLFWwindow* window) const;
    glm::vec3 worldRayDir(const InputManager& input, const Camera& camera, GLFWwindow* window) const;
    glm::vec3 worldRayOrigin(const Camera& camera) const;
    int hitTestCube(const glm::vec3& rayOrigin, const glm::vec3& rayDir, int cubeIndex, float& outT) const;
    int hitTestFace(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& outT) const;
    float snapValue(float val) const;

    std::vector<SceneObject> objects;
    SliceMode sliceMode = SliceMode::Axis;
    int diagonalCutCorner = 0;
    
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
    float placementHeight = 1.0f;
    float scrollAccum = 0.0f;
    int surfaceHitCube = -1;

    int dragFace = -1;

    int sliceAxis = -1;
    bool sliceActive = false;
    float slicePosition = 0.0f;

    bool moving = false;
    glm::vec3 moveOffset{0.0f};
    glm::vec3 activeColor = glm::vec3(1.0f); 

    // multi-select and clipboard
    std::unordered_set<int> multiSelected;
    std::vector<SceneObject> clipBoard;
    glm::vec3 clipboardOrigin{0.0f};

    // paste preview
    bool pasting = false;
    std::vector<SceneObject> pastePreview;
    
    mutable SpatialIndex spatialIndex{1.0f};
    mutable bool spatialDirty = true;
    
};
