#include "ObjModelMenu.h"
#include "ObjLoader.h"
#include "SceneEditor.h"
#include "Log.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <iostream>

ObjModelMenu::ObjModelMenu(MeshRenderer& meshRenderer)
    : meshRenderer(meshRenderer)
    , editor(editor)
{
    refreshModelList();
}

void ObjModelMenu::refreshModelList()
{
    modelFiles.clear();
    statusMessage = "";
    
    try{
        if (!std::filesystem::exists(assetsPath)){
            statusMessage = "Assets path not found: " + assetsPath;
            return;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(assetsPath)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".obj" || ext == ".OBJ") {
                    modelFiles.push_back(entry.path().filename().string());
                }
            }
        }
        if (modelFiles.empty()) {
            statusMessage = "No .obj files found in " + assetsPath;
        } else {
            statusMessage = std::to_string(modelFiles.size()) + " model(s) found";
        }
    } catch (const std::exception& e){
        statusMessage = "Error scanning directory: " + std::string(e.what());
    }
}

void ObjModelMenu::loadSelectedModel()
{
    if (selectedModel.empty()) {
        statusMessage = "No model selected";
        return;
    }

    std::string fullPath = assetsPath + "/" + selectedModel;

    try {
        Mesh meshData = loadObj(fullPath);

        if (meshData.vertices.empty()) {
            statusMessage = "Loaded mesh is empty: " + selectedModel;
            return;
        }

        const size_t vertexCount = meshData.vertices.size();
        Log::info("Adding large mesh (" + std::to_string(vertexCount) + " verts) directly to MeshRenderer");

        // 1. Register the mesh geometry with MeshRenderer
        uint32_t meshId = meshRenderer.loadMesh(meshData, selectedModel);

        // 2. Add an instance for rendering
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
        meshRenderer.addInstance(meshId, transform, glm::vec3(1.0f, 1.0f, 1.0f));

        statusMessage = "Successfully rendered: " + selectedModel
                      + " (" + std::to_string(vertexCount) + " verts)";

        Log::info("Large mesh rendered successfully with ID " + std::to_string(meshId));

    } catch (const std::exception& e) {
        statusMessage = "Failed to load " + selectedModel + ": " + e.what();
        Log::error("ObjModelMenu load failed: " + std::string(e.what()));
    } catch (...) {
        statusMessage = "Failed to load " + selectedModel + ": unknown error";
        Log::error("ObjModelMenu load failed with unknown exception");
    }
}

void ObjModelMenu::draw()
{
    if (!visible) {
            return;                    // Early exit - critical for performance
        }
        
        ImGui::SetNextWindowSize(ImVec2(420, 520), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("OBJ Model Loader", &visible, ImGuiWindowFlags_NoCollapse)) {
            ImGui::End();
            return;
        }
        
        ImGui::Text("Assets Directory:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s", assetsPath.c_str());
        
        if (ImGui::Button("Refresh List")) {
            refreshModelList();
        }
        
        ImGui::Separator();
        
        ImGui::Text("Available Models (%zu)", modelFiles.size());
        ImGui::BeginChild("ModelList", ImVec2(0, 300), true);
        
        for (const auto& file : modelFiles) {
            bool isSelected = (selectedModel == file);
            if (ImGui::Selectable(file.c_str(), isSelected)) {
                selectedModel = file;
            }
        }
        
        ImGui::EndChild();
        ImGui::Separator();
        
        if (ImGui::Button("Load Selected Model", ImVec2(-1, 0))) {
            if (!selectedModel.empty()) {
                loadSelectedModel();
            } else {
                statusMessage = "Please select a model first";
            }
        }
        
        if (!statusMessage.empty()) {
            ImGui::TextWrapped("%s", statusMessage.c_str());
        }
        
        ImGui::End();
}
