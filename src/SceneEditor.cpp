#include "SceneEditor.h"
#include "Log.h"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <string>
#include <algorithm>

float SceneEditor::snapValue(float val) const {
    return std::round(val / gridSnap) * gridSnap;
}

glm::vec3 SceneEditor::worldRayOrigin(const Camera& camera) const {
    return camera.position();
}

glm::vec3 SceneEditor::worldRayDir(const InputManager& input, const Camera& camera, GLFWwindow* window) const {
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
    return glm::normalize(worldFar - worldNear);
}

glm::vec3 SceneEditor::raycastGrid(const InputManager& input, const Camera& camera, GLFWwindow* window) const {
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

int SceneEditor::hitTestCube(const glm::vec3& rayOrigin, const glm::vec3& rayDir, int cubeIndex, float& outT) const {
    if (cubeIndex < 0 || cubeIndex >= static_cast<int>(cubes.size())) return -1;

    const AABB& box = cubes[cubeIndex];
    float tMin = -1e30f;
    float tMax = 1e30f;
    int entryFace = -1;

    for (int axis = 0; axis < 3; ++axis) {
        float invD = 1.0f / rayDir[axis];
        float t0 = (box.min[axis] - rayOrigin[axis]) * invD;
        float t1 = (box.max[axis] - rayOrigin[axis]) * invD;

        int faceMin = axis * 2 + 1;
        int faceMax = axis * 2;

        if (invD < 0.0f) {
            std::swap(t0, t1);
            std::swap(faceMin, faceMax);
        }

        if (t0 > tMin) {
            tMin = t0;
            entryFace = faceMin;
        }
        if (t1 < tMax) {
            tMax = t1;
        }
        if (tMax < tMin) return -1;
    }

    if (tMin < 0.0f) return -1;
    outT = tMin;
    return entryFace;
}

int SceneEditor::hitTestFace(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& outT) const {
    if (selected < 0) return -1;
    return hitTestCube(rayOrigin, rayDir, selected, outT);
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
        glm::vec3 rayOrigin = worldRayOrigin(camera);
        glm::vec3 rayDir = worldRayDir(input, camera, window);

        if (dragFace >= 0 && selected >= 0) {
            if (input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                int ax = dragFace / 2;
                bool isMax = (dragFace % 2) == 0;

                glm::vec3 planeNormal(0.0f);
                if (ax == 1) {
                    glm::vec3 camFlat = glm::normalize(glm::vec3(rayDir.x, 0.0f, rayDir.z));
                    planeNormal = camFlat;
                } else {
                    planeNormal[ax == 0 ? 2 : 0] = 1.0f;
                }

                AABB& sel = cubes[selected];
                glm::vec3 facePoint(0.0f);
                facePoint[ax] = isMax ? sel.max[ax] : sel.min[ax];

                float denom = glm::dot(planeNormal, rayDir);
                if (std::abs(denom) > 0.001f) {
                    float t = glm::dot(facePoint - rayOrigin, planeNormal) / denom;
                    if (t > 0.0f) {
                        glm::vec3 worldPoint = rayOrigin + rayDir * t;
                        float newVal = snapValue(worldPoint[ax]);

                        if (isMax) {
                            if (newVal > sel.min[ax] + gridSnap * 0.5f) {
                                sel.max[ax] = newVal;
                            }
                        } else {
                            if (newVal < sel.max[ax] - gridSnap * 0.5f) {
                                sel.min[ax] = newVal;
                            }
                        }
                    }
                }
            } else {
                Log::info("Face drag finished");
                dragFace = -1;
            }
        } else {
            if (input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                if (selected >= 0) {
                    float faceT = 0.0f;
                    int face = hitTestFace(rayOrigin, rayDir, faceT);
                    if (face >= 0) {
                        dragFace = face;
                        int ax = face / 2;
                        bool isMax = (face % 2) == 0;
                        const char* axNames[] = { "X", "Y", "Z" };
                        Log::info(std::string("Dragging ") + (isMax ? "+" : "-") + axNames[ax] + " face");
                        return;
                    }
                }

                selected = -1;
                dragFace = -1;
                float closestT = 1e30f;
                for (int i = 0; i < static_cast<int>(cubes.size()); ++i) {
                    float t = 0.0f;
                    int face = hitTestCube(rayOrigin, rayDir, i, t);
                    if (face >= 0 && t < closestT) {
                        closestT = t;
                        selected = i;
                    }
                }
                if (selected >= 0) {
                    Log::info("Selected cube " + std::to_string(selected));
                }
            }
        }

        if (selected >= 0 && input.wasKeyJustPressed(GLFW_KEY_DELETE)) {
            Log::info("Deleted cube " + std::to_string(selected));
            cubes.erase(cubes.begin() + selected);
            selected = -1;
            dragFace = -1;
        }
        if (selected >= 0 && input.wasKeyJustPressed(GLFW_KEY_BACKSPACE)) {
            Log::info("Deleted cube " + std::to_string(selected));
            cubes.erase(cubes.begin() + selected);
            selected = -1;
            dragFace = -1;
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

        if (dragFace >= 0) {
            const char* faceNames[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
            ImGui::Text("Dragging: %s face", faceNames[dragFace]);
        }
    }
    if (ImGui::Button("Clear All")) {
        cubes.clear();
        selected = -1;
        Log::info("All cubes cleared");
    }
    ImGui::Separator();
    ImGui::Text("[1] Place  [2] Select  [Del] Delete");
}
