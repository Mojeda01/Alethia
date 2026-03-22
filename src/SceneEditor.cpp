#include "SceneEditor.h"
#include "Log.h"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <string>
#include <algorithm>
#include <fstream>

float SceneEditor::snapValue(float val) const {
    return std::floor(val / gridSnap) * gridSnap;
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

int SceneEditor::hitTestSurface(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& outY) const 
{
    float closestT = 1e30f;
    int hitIndex = -1;

    for (int i = 0; i < static_cast<int>(cubes.size()); ++i){
        float t = 0.0f;
        int face = hitTestCube(rayOrigin, rayDir, i, t);
        if (face == 2 && t < closestT) {
            closestT = t;
            hitIndex = i;
        }
    }
    if (hitIndex >= 0) {
        outY = cubes[hitIndex].max.y;
    }
    return hitIndex;
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
        // fire ray against existing cube top faces first
        glm::vec3 rayOrigin = worldRayOrigin(camera);
        glm::vec3 rayDir = worldRayDir(input, camera, window);

        float surfaceY = 0.0f;
        surfaceHitCube = hitTestSurface(rayOrigin, rayDir, surfaceY);

        if (!dragging){
            // hovering - show context-aware highlighting
            highlightValid = true;

            if (surfaceHitCube >= 0) {
                // hovering over a cube top face — highlight the entire top face
                const AABB& below = cubes[surfaceHitCube];
                highlightCellMin = glm::vec3(below.min.x, below.max.y, below.min.z);
                highlightCellMax = glm::vec3(below.max.x, below.max.y + 0.02f, below.max.z);
            } else {
                // hovering over ground — highlight single grid cell
                highlightCellMin = glm::vec3(snappedX - gridSnap * 0.5f, 0.0f,  snappedZ - gridSnap * 0.5f);
                highlightCellMax = glm::vec3(snappedX + gridSnap * 0.5f, 0.02f, snappedZ + gridSnap * 0.5f);
            }
            if (input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                placementY = (surfaceHitCube >= 0) ? surfaceY : 0.0f;
                dragging   = true;
                dragStart  = glm::vec3(snappedX, placementY, snappedZ);
                Log::info("Drag started at Y=" + std::to_string(placementY));
            }
        }

        if (dragging) {
            // suppress hover highlight during drag — reduces visual noise
            highlightValid = false;

            float minX = std::min(dragStart.x, snappedX);
            float maxX = std::max(dragStart.x, snappedX);
            float minZ = std::min(dragStart.z, snappedZ);
            float maxZ = std::max(dragStart.z, snappedZ);
            
            // respond after half a grid cell instead of a full cell.
            if (maxX - minX < gridSnap * 0.5f) maxX = minX + gridSnap;  
            if (maxZ - minZ < gridSnap * 0.5f) maxZ = minZ + gridSnap;
            
            // preview is exactly what you see — no extra gridSnap added
            preview.min = glm::vec3(minX, placementY, minZ);
            preview.max = glm::vec3(maxX, placementY + gridSnap, maxZ);  

            if (!input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                cubes.push_back(preview);
                Log::info("Cube placed: Y=" + std::to_string(placementY) +
                            " (" + std::to_string(preview.min.x) + ", " +
                            std::to_string(preview.min.z) + ") to (" +
                            std::to_string(preview.max.x) + ", " +
                            std::to_string(preview.max.z) + ")");
                dragging = false;
                surfaceHitCube = -1;
            }
        }
    } 
    else if (tool == Tool::Select) {
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
    else if (tool == Tool::Slice) {
            if (selected < 0) {
                tool = Tool::Select;
                Log::warn("No cube selected — switching to Select");
            } else {
                glm::vec3 rayOrigin = worldRayOrigin(camera);
                glm::vec3 rayDir = worldRayDir(input, camera, window);

                if (!sliceActive) {
                    if (input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                        float faceT = 0.0f;
                        int face = hitTestFace(rayOrigin, rayDir, faceT);
                        if (face >= 0) {
                            sliceAxis = face / 2;
                            sliceActive = true;
                            const AABB& sel = cubes[selected];
                            slicePosition = (sel.min[sliceAxis] + sel.max[sliceAxis]) * 0.5f;
                            slicePosition = snapValue(slicePosition);
                            const char* axNames[] = { "X", "Y", "Z" };
                            Log::info(std::string("Slice axis: ") + axNames[sliceAxis]);
                        }
                    }
                } else {
                    glm::vec3 planeNormal(0.0f);
                    if (sliceAxis == 1) {
                        glm::vec3 camFlat = glm::normalize(glm::vec3(rayDir.x, 0.0f, rayDir.z));
                        planeNormal = camFlat;
                    } else {
                        planeNormal[sliceAxis == 0 ? 2 : 0] = 1.0f;
                    }

                    const AABB& sel = cubes[selected];
                    glm::vec3 facePoint(0.0f);
                    facePoint[sliceAxis] = slicePosition;

                    float denom = glm::dot(planeNormal, rayDir);
                    if (std::abs(denom) > 0.001f) {
                        float t = glm::dot(facePoint - rayOrigin, planeNormal) / denom;
                        if (t > 0.0f) {
                            glm::vec3 worldPoint = rayOrigin + rayDir * t;
                            float newPos = snapValue(worldPoint[sliceAxis]);
                            float minBound = sel.min[sliceAxis] + gridSnap;
                            float maxBound = sel.max[sliceAxis] - gridSnap;
                            if (newPos >= minBound && newPos <= maxBound) {
                                slicePosition = newPos;
                            }
                        }
                    }
                    if (input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                        AABB cubeA = sel;
                        AABB cubeB = sel;
                        cubeA.max[sliceAxis] = slicePosition;
                        cubeB.min[sliceAxis] = slicePosition;

                        int oldSelected = selected;
                        cubes.push_back(cubeA);
                        cubes.push_back(cubeB);
                        cubes.erase(cubes.begin() + oldSelected);

                        Log::info("Cube sliced along " + std::string(1, "XYZ"[sliceAxis]) +
                                " at " + std::to_string(slicePosition));

                        selected = -1;
                        sliceActive = false;
                        sliceAxis = -1;
                        tool = Tool::Select;
                    }

                    if (input.wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
                        sliceActive = false;
                        sliceAxis = -1;
                        tool = Tool::Select;
                        Log::info("Slice cancelled");
                    }
                }
            }
        }
        else if (tool == Tool::Move) {
            if (selected < 0) {
                tool = Tool::Select;
                Log::warn("No cube selected — switching to Select");
            } else {
                if (input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                    AABB& sel = cubes[selected];
                    glm::vec3 center = sel.center();
                    moveOffset = glm::vec3(center.x - snappedX, 0.0f, center.z - snappedZ);
                    moving = true;
                } 
                if (moving && input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                    AABB& sel = cubes[selected];
                    glm::vec3 sz = sel.size();
                    if (input.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || input.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)) {
                        glm::vec3 rayOrigin = worldRayOrigin(camera);
                        glm::vec3 rayDir = worldRayDir(input, camera, window);

                        glm::vec3 camFlat = glm::normalize(glm::vec3(rayDir.x, 0.0f, rayDir.z));
                        glm::vec3 cubeCenter = sel.center();
                        float denom = glm::dot(camFlat, rayDir);
                        if (std::abs(denom) > 0.001f) {
                            float t = glm::dot(cubeCenter - rayOrigin, camFlat) / denom;
                            if (t > 0.0f) {
                                glm::vec3 worldPoint = rayOrigin + rayDir * t;
                                float newY = snapValue(worldPoint.y - sz.y * 0.5f);
                                if (newY < 0.0f) newY = 0.0f;
                                sel.min.y = newY;
                                sel.max.y = newY + sz.y;
                            }
                        }
                    } else {
                        float newCenterX = snappedX + moveOffset.x;
                        float newCenterZ = snappedZ + moveOffset.z;
                        newCenterX = snapValue(newCenterX);
                        newCenterZ = snapValue(newCenterZ);
                        sel.min.x = newCenterX - sz.x * 0.5f;
                        sel.max.x = newCenterX + sz.x * 0.5f;
                        sel.min.z = newCenterZ - sz.z * 0.5f;
                        sel.max.z = newCenterZ + sz.z * 0.5f;
                    }
                }

                if (moving && !input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                    Log::info("Cube moved");
                    moving = false;
                }

                if (input.wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
                    moving = false;
                    tool = Tool::Select;
                    Log::info("Move cancelled");
                }
            }
        }

    if (!input.imguiWantsKeyboard() && input.wasKeyJustPressed(GLFW_KEY_1)) { 
        tool = Tool::Place;
        selected = -1;
        Log::info("Tool: Place");
    }
    if (!input.imguiWantsKeyboard() && input.wasKeyJustPressed(GLFW_KEY_2)) { 
        tool = Tool::Select;
        dragging = false;
        Log::info("Tool: Select");
    }
    if (!input.imguiWantsKeyboard() && input.wasKeyJustPressed(GLFW_KEY_3)) { 
        if (selected >= 0) {
            tool = Tool::Slice;
            sliceActive = false;
            sliceAxis = -1;
            Log::info("Tool: Slice");
        } else {
            Log::warn("Select a cube first before slicing");
        }
    }
    if (!input.imguiWantsKeyboard() && input.wasKeyJustPressed(GLFW_KEY_4)) { 
        if (selected >= 0) {
            tool = Tool::Move;
            moving = false;
            Log::info("Tool: Move");
        } else {
            Log::warn("Select a cube first before moving");
        }
    }
}

void SceneEditor::newProject() {
    cubes.clear();
    selected = -1;
    dragFace = -1;
    sliceActive = false;
    sliceAxis = -1;
    moving = false;
    tool = Tool::Place;
    gridSnap = 1.0f;
    Log::info("New project created");
}

bool SceneEditor::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        Log::error("Failed to open file for saving: " + filename);
        return false;
    }

    file << "alethia v1 snap=" << gridSnap << "\n";
    for (const auto& cube : cubes) {
        file << cube.min.x << " " << cube.min.y << " " << cube.min.z << " "
             << cube.max.x << " " << cube.max.y << " " << cube.max.z << "\n";
    }

    file.close();
    Log::info("Saved " + std::to_string(cubes.size()) + " cubes to " + filename);
    return true;
}

bool SceneEditor::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Log::error("Failed to open file for loading: " + filename);
        return false;
    }

    std::string header;
    std::getline(file, header);

    if (header.find("alethia v1") == std::string::npos) {
        Log::error("Invalid file format: " + filename);
        return false;
    }

    auto snapPos = header.find("snap=");
    if (snapPos != std::string::npos) {
        gridSnap = std::stof(header.substr(snapPos + 5));
    }

    std::vector<AABB> loaded;
    float minX, minY, minZ, maxX, maxY, maxZ;
    while (file >> minX >> minY >> minZ >> maxX >> maxY >> maxZ) {
        AABB cube;
        cube.min = glm::vec3(minX, minY, minZ);
        cube.max = glm::vec3(maxX, maxY, maxZ);
        loaded.push_back(cube);
    }

    file.close();

    cubes = std::move(loaded);
    selected = -1;
    dragFace = -1;
    sliceActive = false;
    sliceAxis = -1;
    moving = false;
    tool = Tool::Select;

    Log::info("Loaded " + std::to_string(cubes.size()) + " cubes from " + filename);
    return true;
}

void SceneEditor::drawUI() {
    const char* toolNames[] = { "Place", "Select", "Slice", "Move"};
    int toolIdx = static_cast<int>(tool);
    if (ImGui::Combo("Tool", &toolIdx, toolNames, 4)) {
        tool = static_cast<Tool>(toolIdx);
        if (tool == Tool::Place) selected = -1;
        if (tool == Tool::Select) dragging = false;
        if (tool == Tool::Slice && selected < 0){
            tool = Tool::Select;
            Log::warn("Select a cube first before slicing");
        }
        if (tool == Tool::Move && selected < 0) {
            tool = Tool::Select;
            Log::warn("Select a cube first before moving");
        }
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
        if (sliceActive){
            const char* axNames[] = { "X", "Y", "Z" };
            ImGui::Text("Slicing along %s at %.2f", axNames[sliceAxis], slicePosition);
        }
    }
    if (ImGui::Button("Clear All")) {
        cubes.clear();
        selected = -1;
        Log::info("All cubes cleared");
    }
    ImGui::Separator();
    ImGui::Text("[1] Place  [2] Select [3] Slice [4] Move  [Del] Delete \n[Shift+Drag] Move Y-axis");
}
