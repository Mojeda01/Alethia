#pragma once

#include "Mesh.h"
#include "MeshRenderer.h"
#include "SceneEditor.h"

#include <string>
#include <vector>
#include <filesystem>

class ObjModelMenu {
public:
    explicit ObjModelMenu(MeshRenderer& meshRenderer);
    void draw();                    // Call every frame when visible
    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }
    void setAssetsPath(const std::string& path) { assetsPath = path; refreshModelList(); }
private:
    MeshRenderer& meshRenderer;
    SceneEditor& editor;
    bool visible = true;
    std::string assetsPath = "assets/models";
    std::vector<std::string> modelFiles;
    std::string selectedModel;
    std::string statusMessage;
    void refreshModelList();
    void loadSelectedModel();
};
