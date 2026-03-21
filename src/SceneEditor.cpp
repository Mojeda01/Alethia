#include "SceneEditor.h"
#include "Log.h"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <string>

float SceneEditor::snapValue(float val) const {
    return std::round(val / gridSnap) * gridSnap;
}

glm::vec3 SceneEditor::raycastGrid(const InputManager& input, const Camera& camera, GLFWwindow* window) const 
{
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);

    float ndcX = (2.0f * static_cast<float>(input.mouseX())) / static_cast<float>(winW) - 1.0f;
    float ndcY = 1.0f - (2.0f * static_cast<float>(input.mouseY())) / static_cast<float>(winH);

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

void SceneEditor::update(const InputManager& input, const Camera& camera, GLFWwindow* window) {
    highlightValid = false;

    if (!input.inUIMode()) return;
    if (input.imguiWantsMouse()) return;

    glm::vec3 hit = raycastGrid(input, camera, window);
    if (hit.y < -9999.0f) return;

    float snappedX = snapValue(hit.x);
    float snappedZ = snapValue(hit.z);

    if (tool == Tool::Place) {
         highlightValid = true;
         highlightCellMin = glm::vec3(snappedX - gridSnap * 0.5f, 0.0f, snappedZ - gridSnap * 0.5f);
         highlightCellMax = glm::vec3(snappedX + gridSnap * 0.5f, 0.01f, snappedZ + gridSnap * 0.5f);

         if (input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            dragging = true;
            dragStart = glm::vec3(snappedX, 0.0f, snappedZ);
         }

         if (dragging) {
            float minX = std::min(dragStart.x, snappedX);
            float maxX = std::max(dragStart.x, snappedX);
            float minZ = std::min(dragStart.z, snappedZ);
            float maxZ = std::max(dragStart.z, snappedZ);

            if (maxX - minX < gridSnap) maxX = minX + gridSnap;
            if (maxZ - minZ < gridSnap) maxZ = minZ + gridSnap;

            preview.min = glm::vec3(minX - gridSnap * 0.5f, 0.0f, minZ - gridSnap * 0.5f);
            preview.max = glm::vec3(maxX + gridSnap * 0.5f, gridSnap, maxZ + gridSnap * 0.5f);

            highlightCellMin = preview.min;
            highlightCellMax = glm::vec3(preview.max.x, 0.01f, preview.max.z);

            if (!input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                cubes.push_back(preview);
                 Log::info("Cube placed: (" +
                        std::to_string(preview.min.x) + ", " + std::to_string(preview.min.z) + ") to (" +
                        std::to_string(preview.max.x) + ", " + std::to_string(preview.max.z) + ")");
                 dragging = false;
            }
         }
    } else if (tool == Tool::Select) {
        if (input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            selected = -1;
            glm::vec3 rayOrigin = camera.position();
            glm::vec3 rayDir = glm::normalize(hit - rayOrigin);

            float closestT = 1e30f;
            for (int i = 0; i < static_cast<int>(cubes.size()); ++i) {
                const AABB& box = cubes[i];
                float tMin = 0.0f, tMax = 1e30f;
                bool miss = false;

                for (int axis = 0; axis < 3; ++axis) {
                    float invD = 1.0f / rayDir[axis];
                    float t0 = (box.min[axis] - rayOrigin[axis]) * invD;
                    float t1 = (box.max[axis] - rayOrigin[axis]) * invD;
                    if (invD < 0.0f) std::swap(t0, t1);
                    tMin = std::max(tMin, t0);
                    tMax = std::min(tMax, t1);
                    if (tMax < tMin) { miss = true; break; }
                }
                if (!miss && tMin < closestT && tMin > 0.0f) {
                    closestT = tMin;
                    selected = i;
                }
            }
            if (selected >= 0) {
                Log::info("Selected cube " + std::to_string(selected));
            }
        }
        if (selected >= 0 && input.wasKeyJustPressed(GLFW_KEY_DELETE)) {
            Log::info("Deleted cube " + std::to_string(selected));
            cubes.erase(cubes.begin() + selected);
            selected = -1;
        }
        if (input.wasKeyJustPressed(GLFW_KEY_BACKSPACE)) {
            if (selected >= 0) {
                Log::info("Deleted cube " + std::to_string(selected));
                cubes.erase(cubes.begin() + selected);
                selected = -1;
            }
        }
    }
    if (input.wasKeyJustPressed(GLFW_KEY_1)) {
        tool = Tool::Place;
        selected = -1;
        Log::info("Tool: Place");
    }
    if (input.wasKeyJustPressed(GLFW_KEY_2)) {
        tool = Tool::Select;
        dragging = false;
        Log::info("Tool: Select");
    }
}

void SceneEditor::drawUI() {
    const char* toolNames[] = { "Place", "Select" };
    int toolIdx = static_cast<int>(tool);
    if (ImGui::Combo("Tool", &toolIdx, toolNames, 2)) {
        tool = static_cast<Tool>(toolIdx);
        if (tool == Tool::Place) selected = -1;
        if (tool == Tool::Select) dragging = false;
    }
    ImGui::SliderFloat("Snap", &gridSnap, 0.125f, 4.0f, "%.3f");
    ImGui::Separator();
    ImGui::Text("Cubes: %zu", cubes.size());
    if (selected >= 0) {
        ImGui::Text("Selected: %d", selected);
        AABB& sel = cubes[selected];
        glm::vec3 pos = sel.center();
        glm::vec3 sz = sel.size();

        if (ImGui::DragFloat3("Position", &pos.x, gridSnap, -500.0f, 500.0f, "%.2f")) {
            glm::vec3 half = sz * 0.5f;
            sel.min = pos - half;
            sel.max = pos + half;
        }

        if (ImGui::DragFloat("Width (X)", &sz.x, gridSnap, gridSnap, 500.0f, "%.2f")) {
            if (sz.x < gridSnap) sz.x = gridSnap;
            float cx = sel.center().x;
            sel.min.x = cx - sz.x * 0.5f;
            sel.max.x = cx + sz.x * 0.5f;
        }

        if (ImGui::DragFloat("Height (Y)", &sz.y, gridSnap, gridSnap, 500.0f, "%.2f")) {
            if (sz.y < gridSnap) sz.y = gridSnap;
            sel.max.y = sel.min.y + sz.y;
        }

        if (ImGui::DragFloat("Depth (Z)", &sz.z, gridSnap, gridSnap, 500.0f, "%.2f")) {
            if (sz.z < gridSnap) sz.z = gridSnap;
            float cz = sel.center().z;
            sel.min.z = cz - sz.z * 0.5f;
            sel.max.z = cz + sz.z * 0.5f;
        }
        ImGui::Separator();
        ImGui::Text("Min: %.2f, %.2f, %.2f", sel.min.x, sel.min.y, sel.min.z);
        ImGui::Text("Max: %.2f, %.2f, %.2f", sel.max.x, sel.max.y, sel.max.z);
    }
    if (ImGui::Button("Clear All")) {
        cubes.clear();
        selected = -1;
        Log::info("All cubes cleared");
    }
    ImGui::Separator();
    ImGui::Text("[1] Place  [2] Select  [Del] Delete");
}
