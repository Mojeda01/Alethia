#include "SceneEditor.h"
#include "SpatialIndex.h"
#include "Log.h"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>

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
    if (cubeIndex < 0 || cubeIndex >= static_cast<int>(objects.size())) return -1;

    const AABB& box = objects[cubeIndex].boundingAABB();
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

void SceneEditor::clearSelection() {
    selected = -1;
    multiSelected.clear();
    dragFace = -1;
};

void SceneEditor::eraseAndRemap(int index) {
    objects.erase(objects.begin() + index);

    // rebuild multiSelected with shifted indices
    std::unordered_set<int> remapped;
    for (int idx : multiSelected) {
        if (idx < index) {
            remapped.insert(idx);
        } else if (idx == index) {
            // this cube was erased — drop it
        } else {
            remapped.insert(idx - 1);
        }
    }
    multiSelected = std::move(remapped);

    // fix selected
    if (selected == index) {
        selected = -1;
        dragFace = -1;
    } else if (selected > index) {
        selected -= 1;
    }
}

void SceneEditor::pushSnapshot() {
    // Guard against invalid state
    if (historyCursor < 0 || history.empty()) {
        history.clear();
        history.push_back(objects);
        historyCursor = 0;
        sceneVer++;
        return;
    }

    // Trim any redo history beyond current cursor
    while (history.size() > static_cast<size_t>(historyCursor + 1)) {
        history.pop_back();
    }

    // Enforce max history size
    if (history.size() >= MAX_HISTORY) {
        history.pop_front();
        if (historyCursor > 0) historyCursor--;
    }

    // Push a copy of current objects
    history.push_back(objects);
    historyCursor = static_cast<int>(history.size()) - 1;
    sceneVer++;
}

void SceneEditor::undo() {
    if (!canUndo()) {
        Log::warn("Nothing to undo");
        return;
    }
    historyCursor--;
    objects = history[historyCursor];
    clearSelection();
    Log::info("Redo — " + std::to_string(objects.size()) + " object(s)");
}

void SceneEditor::redo() {
    if (!canRedo()) {
        Log::warn("Nothing to redo");
        return;
    }
    historyCursor++;
    objects = history[historyCursor];
    clearSelection();
    Log::info("Undo — " + std::to_string(objects.size()) + " object(s)");
}

void SceneEditor::buildPastePreview(float targetX, float targetY, float targetZ) {
    pastePreview.clear();
    if (clipBoard.empty()) return;

    glm::vec3 offset(
        snapValue(targetX) - clipboardOrigin.x,
        targetY - clipboardOrigin.y,
        snapValue(targetZ) - clipboardOrigin.z
    );
    for (const SceneObject& src : clipBoard) {
        SceneObject dst = src;
        if (dst.type == ShapeType::Box) {
            dst.box.min += offset;
            dst.box.max += offset;
        } else {
            dst.prism.v0 += offset;
            dst.prism.v1 += offset;
            dst.prism.v2 += offset;
        }
        pastePreview.push_back(dst);
    }
}

int SceneEditor::hitTestSurface(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& outY) const 
{
    if (spatialDirty) {
        spatialIndex.rebuild(objects);
        spatialDirty = false;
    }
    
    float closestT = 1e30f;
    float hitIndex = -1;
    
    glm::vec3 testPoint = rayOrigin + rayDir * 10.0f;
    auto candidates = spatialIndex.queryPoint(testPoint);
    
    for (int i : candidates) {
        float t = 0.0f;
        int face = hitTestCube(rayOrigin, rayDir, i, t);
        if (face >= 0 && t < closestT) {
            closestT = t;
            hitIndex = i;
        }
    }
    
    if (hitIndex >= 0) {
        outY = objects[hitIndex].boundingAABB().max.y;
    }
    return hitIndex;
}

void SceneEditor::update(const InputManager& input, const Camera& camera, GLFWwindow* window) {
    highlightValid = false;

    if (!input.inUIMode()) return;
    if (input.imguiWantsMouse() && !dragging) return;

    glm::vec3 hit = raycastGrid(input, camera, window);
    if (hit.y < -9999.0f && !dragging) return;

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
            
            double sv = input.scrollDelta();
            Log::info("scroll read: " + std::to_string(sv) +
                      " accum: " + std::to_string(scrollAccum) +
                      " height: " + std::to_string(placementHeight));
            scrollAccum += static_cast<float>(input.scrollDelta());

            if (surfaceHitCube >= 0) {
                // hovering over a cube top face — highlight the entire top face
                const AABB& below = objects[surfaceHitCube].boundingAABB();
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
            
            // accumulate scroll and step height in gridSnap increments
            scrollAccum += static_cast<float>(input.scrollDelta());
            const float scrollThreshold = 0.5f;
            while (scrollAccum >= scrollThreshold) {
                placementHeight += gridSnap;
                scrollAccum -= scrollThreshold;
            }
            while (scrollAccum <= -scrollThreshold) {
                placementHeight -= gridSnap;
                if (placementHeight < gridSnap) placementHeight = gridSnap;
                scrollAccum += scrollThreshold;
            }
            
            // preview is exactly what you see — no extra gridSnap added
            preview.min = glm::vec3(minX, placementY, minZ);
            preview.max = glm::vec3(maxX, placementY + placementHeight, maxZ);

            if (!input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) { 
                objects.push_back(SceneObject::fromBox(preview));
                int newIndex = static_cast<int>(objects.size()) - 1;
                objects[newIndex].box.color = activeColor;
                multiSelected.clear();
                multiSelected.insert(newIndex);
                selected = newIndex;
                pushSnapshot();
                
                spatialDirty = true;
                
                Log::info("Cube placed: Y=" + std::to_string(placementY) +
                            " (" + std::to_string(preview.min.x) + ", " +
                            std::to_string(preview.min.z) + ") to (" +
                            std::to_string(preview.max.x) + ", " +
                            std::to_string(preview.max.z) + ")");
                dragging = false;
                surfaceHitCube = -1;
                placementHeight = gridSnap;
                scrollAccum = 0.0f;
            }
        }
    } 
    else if (tool == Tool::Select) {
        glm::vec3 rayOrigin = worldRayOrigin(camera);
        glm::vec3 rayDir = worldRayDir(input, camera, window);

        if (dragFace >= 0 && selected >= 0) {
            if (objects[selected].type == ShapeType::Prism) {
                dragFace = -1;
                Log::warn("Face drag not supported");
            }
            if (dragFace >= 0 && input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                int ax = dragFace / 2;
                bool isMax = (dragFace % 2) == 0;

                glm::vec3 planeNormal(0.0f);
                if (ax == 1) {
                    glm::vec3 camFlat = glm::normalize(glm::vec3(rayDir.x, 0.0f, rayDir.z));
                    planeNormal = camFlat;
                } else {
                    planeNormal[ax == 0 ? 2 : 0] = 1.0f;
                }

                AABB& sel = objects[selected].box;
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
                                if (ax == 0) sel.max.x = newVal;
                                else if (ax == 1) sel.max.y = newVal;
                                else sel.max.z = newVal;
                            }
                        } else {
                            if (newVal < sel.max[ax] - gridSnap * 0.5f) {
                                if (ax == 0) sel.min.x = newVal;
                                else if (ax == 1) sel.min.y = newVal;
                                else sel.min.z = newVal;
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
                // find closest hit 
                float closestT = 1e30f;
                int hitIndex = -1;
                
                if (spatialDirty) {
                    spatialIndex.rebuild(objects);
                    spatialDirty = false;
                }
                
                glm::vec3 testPoint = rayOrigin + rayDir * 50.0f;
                auto candidates = spatialIndex.queryPoint(testPoint);
                auto candidates2 = spatialIndex.queryPoint(rayOrigin);
                for (int i : candidates2) {
                    if (std::find(candidates.begin(), candidates.end(), i) == candidates.end()) {
                        candidates.push_back(i);
                    }
                }
                
                for (int i : candidates) {
                    float t = 0.0f;
                    int face = hitTestCube(rayOrigin, rayDir, i, t);
                    if (face >= 0 && t < closestT) {
                        closestT = t;
                        hitIndex = i;
                    }
                }
                
                bool shiftHeld = input.isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                                    input.isKeyPressed(GLFW_KEY_RIGHT_SHIFT);

                if (hitIndex >= 0) {
                    if (shiftHeld) {
                        // shift+click — toggle in multiSelected, don't change selected
                        if (multiSelected.count(hitIndex)) {
                            multiSelected.erase(hitIndex);
                            Log::info("Deselected cube " + std::to_string(hitIndex));
                        } else {
                            multiSelected.insert(hitIndex);
                            Log::info("Added cube " + std::to_string(hitIndex) +
                                        " to selection (" +
                                        std::to_string(multiSelected.size()) + " total)");
                        }
                    } else {
                        multiSelected.clear();
                        selected = hitIndex;
                        multiSelected.insert(hitIndex);
                        dragFace = -1;
                        Log::info("Selected cube " + std::to_string(selected));
                    }
                } else {
                    clearSelection();
                }
            }
        }

        if (selected >= 0 && input.wasKeyJustPressed(GLFW_KEY_DELETE)) {
            Log::info("Deleted cube " + std::to_string(selected));
            pushSnapshot();
            eraseAndRemap(selected);
        }
        if (selected >= 0 && input.wasKeyJustPressed(GLFW_KEY_BACKSPACE)) {
            Log::info("Deleted cube " + std::to_string(selected));
            pushSnapshot();
            eraseAndRemap(selected);
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
                        if (objects[selected].type == ShapeType::Prism) {
                            Log::warn("Axis-aligned slice not supported for prisms — use diagonal slice on a box first");
                            tool = Tool::Select;
                        } else {
                            sliceAxis = face / 2;
                            sliceActive = true;
                            AABB& sel = objects[selected].box;
                            slicePosition = (sel.min[sliceAxis] + sel.max[sliceAxis]) * 0.5f;
                            slicePosition = snapValue(slicePosition);
                            const char* axNames[] = { "X", "Y", "Z" };
                            Log::info(std::string("Slice axis: ") + axNames[sliceAxis]);
                        }
                    }
                }
            } else {
                if (objects[selected].type == ShapeType::Prism) {
                    sliceActive = false;
                    sliceAxis = -1;
                    tool = Tool::Select;
                    Log::warn("Slice cancelled - prism selected");
                } else {
                    glm::vec3 planeNormal(0.0f);
                    if (sliceAxis == 1) {
                        glm::vec3 camFlat = glm::normalize(glm::vec3(rayDir.x, 0.0f, rayDir.z));
                        planeNormal = camFlat;
                    } else {
                        planeNormal[sliceAxis == 0 ? 2 : 0] = 1.0f;
                    }
                    
                    AABB& sel = objects[selected].box;
                    
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
                        pushSnapshot();
                        int oldSelected = selected;
                        objects.push_back(SceneObject::fromBox(cubeA));
                        objects.push_back(SceneObject::fromBox(cubeB));
                        eraseAndRemap(oldSelected);
                        
                        Log::info("Cube sliced along " + std::string(1, "XYZ"[sliceAxis]) +
                                  " at " + std::to_string(slicePosition));
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
    }
    else if (tool == Tool::Move) {
            if (selected < 0) {
                tool = Tool::Select;
                Log::warn("No cube selected — switching to Select");
            } else {
                if (input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                    glm::vec3 center = objects[selected].boundingAABB().center();
                    moveOffset = glm::vec3(center.x - snappedX, 0.0f, center.z - snappedZ);
                    moving = true;
                } 
                if (moving && input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                    glm::vec3 sz = objects[selected].boundingAABB().size();
                    if (input.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || input.isKeyPressed(GLFW_KEY_RIGHT_SHIFT)) {
                        glm::vec3 rayOrigin = worldRayOrigin(camera);
                        glm::vec3 rayDir = worldRayDir(input, camera, window);

                        glm::vec3 camFlat = glm::normalize(glm::vec3(rayDir.x, 0.0f, rayDir.z));
                        glm::vec3 cubeCenter = objects[selected].boundingAABB().center();
                        float denom = glm::dot(camFlat, rayDir);
                        if (std::abs(denom) > 0.001f) {
                            float t = glm::dot(cubeCenter - rayOrigin, camFlat) / denom;
                            if (t > 0.0f) {
                                glm::vec3 worldPoint = rayOrigin + rayDir * t;
                                float newY = snapValue(worldPoint.y - sz.y * 0.5f);
                                if (newY < 0.0f) newY = 0.0f;
                                if (objects[selected].type == ShapeType::Box) {
                                    objects[selected].box.min.y = newY;
                                    objects[selected].box.max.y = newY + sz.y;
                                } else {
                                    float delta = newY - objects[selected].prism.v0.y;
                                    objects[selected].prism.v0.y += delta;
                                    objects[selected].prism.v1.y += delta;
                                    objects[selected].prism.v2.y += delta;
                                }
                                ++sceneVer;
                            }
                        }
                    } else {
                        float newCenterX = snappedX + moveOffset.x;
                        float newCenterZ = snappedZ + moveOffset.z;
                        newCenterX = snapValue(newCenterX);
                        newCenterZ = snapValue(newCenterZ);
                        if (objects[selected].type == ShapeType::Box) {
                            objects[selected].box.min.x = newCenterX - sz.x * 0.5f;
                            objects[selected].box.max.x = newCenterX + sz.x * 0.5f;
                            objects[selected].box.min.z = newCenterZ - sz.z * 0.5f;
                            objects[selected].box.max.z = newCenterZ + sz.z * 0.5f;
                        } else {
                            glm::vec3 oldCenter = objects[selected].boundingAABB().center();
                            glm::vec3 delta(newCenterX - oldCenter.x, 0.0f,
                                            newCenterZ - oldCenter.z);
                            objects[selected].prism.v0 += delta;
                            objects[selected].prism.v1 += delta;
                            objects[selected].prism.v2 += delta;
                        }
                        ++sceneVer;
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

    // CMC+C - copy all selected cubes
    bool cmdHeld = input.isKeyPressed(GLFW_KEY_LEFT_SUPER) ||
                    input.isKeyPressed(GLFW_KEY_RIGHT_SUPER);

    if (cmdHeld && input.wasKeyJustPressed(GLFW_KEY_C)) {
        if (!multiSelected.empty()) {
            clipBoard.clear();

            // compute bounding center of all selected cubes as clipboard again.
            glm::vec3 totalMin(1e30f), totalMax(-1e30f);
            for (int idx : multiSelected) {
                totalMin = glm::min(totalMin, objects[idx].boundingAABB().min);
                totalMax = glm::max(totalMax, objects[idx].boundingAABB().max);
            }
            clipboardOrigin = (totalMin + totalMax) * 0.5f;
            clipboardOrigin.x = snapValue(clipboardOrigin.x);
            clipboardOrigin.z = snapValue(clipboardOrigin.z);

            for (int idx : multiSelected) {
                clipBoard.push_back(objects[idx]);
            }
            Log::info("Copied " + std::to_string(clipBoard.size()) + " cube(s)");
        } else {
            Log::warn("Nothing selected to copy");
        }
    }

    // CMD+V — begin paste mode
    if (cmdHeld && input.wasKeyJustPressed(GLFW_KEY_V)) {
        if (!clipBoard.empty()) {
            pasting = true;
            Log::info("Paste mode — click to place " + std::to_string(clipBoard.size()) + " cube(s)");
        } else {
            Log::warn("Clipboard is empty");
        }
    }

    bool shiftHeld =    input.isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                        input.isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
    if (cmdHeld && !shiftHeld && input.wasKeyJustPressed(GLFW_KEY_Z)) {
        undo();
    }
    if (cmdHeld && shiftHeld && input.wasKeyJustPressed(GLFW_KEY_Z)) {
        redo();
    }

    // paste mode — active after CMD+V until click or Escape
    if (pasting && !clipBoard.empty()) {
        // surface-aware raycast same as Place tool
        glm::vec3 rayOrigin = worldRayOrigin(camera);
        glm::vec3 rayDir    = worldRayDir(input, camera, window);

        float surfaceY = 0.0f;
        int surfHit = hitTestSurface(rayOrigin, rayDir, surfaceY);
        float pasteY = (surfHit >= 0) ? surfaceY : 0.0f;

        buildPastePreview(snappedX, pasteY, snappedZ);

        if (input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) && !input.imguiWantsMouse()) {
            // commit paste — add all preview cubes to scene
            for (const SceneObject& obj : pastePreview) {
                multiSelected.insert(static_cast<int>(objects.size()));
                objects.push_back(obj);
            }
            // set selected to first pasted cube for immediate editing
            if (!multiSelected.empty()) {
                selected = *multiSelected.begin();
            }
            Log::info("Pasted " + std::to_string(pastePreview.size()) + " cube(s)");
            pasting = false;
            pastePreview.clear();
        }
        if (input.wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            pasting = false;
            pastePreview.clear();
            Log::info("Paste cancelled");
        }
    }

    if (!input.imguiWantsKeyboard() && input.wasKeyJustPressed(GLFW_KEY_1)) { 
        tool = Tool::Place;
        selected = -1;
        placementHeight = gridSnap;
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
    objects.clear();
    clearSelection();
        
    history.clear();
    history.push_back(objects);        // initial empty snapshot
    historyCursor = 0;
    sceneVer = 1;
        
    sliceActive = false;
    sliceAxis = -1;
    moving = false;
    tool = Tool::Place;
    gridSnap = 1.0f;
    clipBoard.clear();
    pastePreview.clear();
    pasting = false;
    spatialDirty = true;
        
    Log::info("New project created");
}

bool SceneEditor::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        Log::error("Failed to open file for saving: " + filename);
        return false;
    }

    file << "alethia v3 snap=" << gridSnap << "\n";
    for (const auto& obj : objects) {
        if (obj.type == ShapeType::Box) {
            const AABB& b = obj.box;
            file << "box "
            << b.min.x << " " << b.min.y << " " << b.min.z << " "
            << b.max.x << " " << b.max.y << " " << b.max.z << " "
            << b.color.r << " " << b.color.g << " " << b.color.b << "\n";
        } else {
            const TriangularPrism& p = obj.prism;
            file << "prism "
            << p.v0.x << " " << p.v0.y << " " << p.v0.z << " "
            << p.v1.x << " " << p.v1.y << " " << p.v1.z << " "
            << p.v2.x << " " << p.v2.y << " " << p.v2.z << " "
            << p.extrudeDir.x << " " << p.extrudeDir.y << " " << p.extrudeDir.z << " "
            << p.color.r << " " << p.color.g << " " << p.color.b << "\n";
        }
    }
    file.close();
    Log::info("Saved " + std::to_string(objects.size()) + " objects to " + filename);
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

    
    bool isV1 = header.find("alethia v1") != std::string::npos;
    bool isV2 = header.find("alethia v2") != std::string::npos;
    bool isV3 = header.find("alethia v3") != std::string::npos;

    if (!isV1 && !isV2 && !isV3) {
        Log::error("Invalid file format: " + filename);
        return false;
    }
    if (isV1) {
        Log::warn("Loading v1 scene — colors defaulted to white");
    }

    auto snapPos = header.find("snap=");
    if (snapPos != std::string::npos) {
        gridSnap = std::stof(header.substr(snapPos + 5));
    }

    std::vector<SceneObject> loaded;
    float minX, minY, minZ, maxX, maxY, maxZ, r, g, b; 
    
    if (isV3) {
        std::string tag;
        while (file >> tag) {
            if (tag == "box") {
                file >> minX >> minY >> minZ >> maxX >> maxY >> maxZ >> r >> g >> b;
                AABB box;
                box.min = glm::vec3(minX, minY, minZ);
                box.max = glm::vec3(maxX, maxY, maxZ);
                box.color = glm::vec3(r, g, b);
                loaded.push_back(SceneObject::fromBox(box));
            } else if (tag == "prism") {
                float v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z;
                float ex, ey, ez;
                file >> v0x >> v0y >> v0z
                >> v1x >> v1y >> v1z
                >> v2x >> v2y >> v2z
                >> ex  >> ey  >> ez
                >> r   >> g   >> b;
                TriangularPrism p;
                p.v0 = glm::vec3(v0x, v0y, v0z);
                p.v1 = glm::vec3(v1x, v1y, v1z);
                p.v2 = glm::vec3(v2x, v2y, v2z);
                p.extrudeDir = glm::vec3(ex, ey, ez);
                p.color = glm::vec3(r, g, b);
                loaded.push_back(SceneObject::fromPrism(p));
            }
        }
    } else if (isV2) {
        while (file >> minX >> minY >> minZ >> maxX >> maxY >> maxZ >> r >> g >> b) {
            AABB box;
            box.min = glm::vec3(minX, minY, minZ);
            box.max = glm::vec3(maxX, maxY, maxZ);
            box.color = glm::vec3(r, g, b);
            loaded.push_back(SceneObject::fromBox(box));
        }
    } else {
        while (file >> minX >> minY >> minZ >> maxX >> maxY >> maxZ) {
            AABB box;
            box.min = glm::vec3(minX, minY, minZ);
            box.max = glm::vec3(maxX, maxY, maxZ);
            box.color = glm::vec3(1.0f);
            loaded.push_back(SceneObject::fromBox(box));
        }
    }
    
    file.close();
    
    objects = std::move(loaded);
    history.clear();
    historyCursor = -1;
    pushSnapshot();
    clearSelection();
    sliceActive = false;
    sliceAxis = -1;
    moving = false;
    pasting = false;
    pastePreview.clear();
    tool = Tool::Select;
    Log::info("Loaded " + std::to_string(objects.size()) + " objects from " + filename);
    
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
    ImGui::Text("Objects: %zu", objects.size());
    if (selected >= 0) {
        ImGui::Text("Selected: %d", selected);
        if (objects[selected].type == ShapeType::Box) {
            AABB& sel = objects[selected].box;
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
        } else {
            AABB bound = objects[selected].boundingAABB();
            glm::vec3 center = bound.center();
            glm::vec3 sz = bound.size();
            ImGui::Text("Type: Triangular Prism");
            ImGui::Text("Center: %.2f, %.2f, %.2f", center.x, center.y, center.z);
            ImGui::Text("Bounds: %.2f x %.2f x %.2f", sz.x, sz.y, sz.z);
            ImGui::Separator();
            ImGui::Text("Min: %.2f, %.2f, %.2f", bound.min.x, bound.min.y, bound.min.z);
            ImGui::Text("Max: %.2f, %.2f, %.2f", bound.max.x, bound.max.y, bound.max.z);
        }

        if (dragFace >= 0) {
            const char* faceNames[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
            ImGui::Text("Dragging: %s face", faceNames[dragFace]);
        }
        if (sliceActive){
            const char* axNames[] = { "X", "Y", "Z" };
            ImGui::Text("Slicing along %s at %.2f", axNames[sliceAxis], slicePosition);
        }
        if (tool == Tool::Slice) {
            ImGui::Separator();
            const char* sliceModeNames[] = {
                "Axis-Aligned", "Diagonal XZ", "Diagonal XY", "Diagonal YZ"
            };
            int sm = static_cast<int>(sliceMode);
            if (ImGui::Combo("Slice Mode", &sm, sliceModeNames, 4)) {
                sliceMode = static_cast<SliceMode>(sm);
            }
            if (sliceMode != SliceMode::Axis) {
                ImGui::SliderInt("Cut Corner", &diagonalCutCorner, 0, 3);
                ImGui::Text("0=min-X/min-Z  1=max-X/min-Z");
                ImGui::Text("2=max-X/max-Z  3=min-X/max-Z");
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button,
                                      ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                if (ImGui::Button("Apply Diagonal Slice", ImVec2(-1, 0))) {
                    applyDiagonalSlice();
                }
                ImGui::PopStyleColor();
            }
        }
    }
    if (ImGui::Button("Clear All")) {
        pushSnapshot();
        objects.clear();
        clearSelection();
        Log::info("All cubes cleared");
    }
    ImGui::Separator();
    if (!clipBoard.empty()) {
        ImGui::Text("Clipboard: %zu cube(s)", clipBoard.size());
    }
    ImGui::Text("History: %d step(s)", historyCursor + 1);
    ImGui::BeginDisabled(!canUndo());
    if (ImGui::Button("Undo")) undo();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!canRedo());
    if (ImGui::Button("Redo")) redo();
    ImGui::EndDisabled();
    if (pasting) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 1.0f, 1.0f), "PASTE MODE — click to place");
        if (ImGui::Button("Cancel Paste")) {
            pasting = false;
            pastePreview.clear();
        }
    }
    if (!multiSelected.empty()) {
        ImGui::Text("Selected: %zu cube(s)", multiSelected.size());
    }
    ImGui::Separator();
    ImGui::Text("[1] Place  [2] Select [3] Slice [4] Move  [Del] Delete\n"
                "[Shift+Click] Multi-select  [CMD+C] Copy  [CMD+V] Paste\n"
                "[Shift+Drag] Move Y-axis");
}

void SceneEditor::applyColorToSelection(const glm::vec3& color) {
    if (multiSelected.empty()) return;
    pushSnapshot();
    for (int idx : multiSelected) {
        if (idx >= 0 && idx < static_cast<int>(objects.size())) {
            objects[idx].setColor(color);
        }
    }
    activeColor = color;
    Log::info("Applied color to " + std::to_string(multiSelected.size()) +
            " cube(s)");

}

bool SceneEditor::getDiagonalSlicePreviewLine(glm::vec3& outA, glm::vec3& outB) const {
    if (selected < 0 || selected >= static_cast<int>(objects.size())) return false;
    if (sliceMode == SliceMode::Axis) return false;
    
    const AABB box = objects[selected].boundingAABB();
    
    // draw the diagonal line across the relevant face based on cutCorner
    if (sliceMode == SliceMode::DiagonalXZ) {
        float y = (box.min.y + box.max.y) * 0.5f;
        switch (diagonalCutCorner % 2) {
            case 0:
                outA = glm::vec3(box.min.x, y, box.max.z);
                outB = glm::vec3(box.max.x, y, box.min.z);
                break;
            default:
                outA = glm::vec3(box.min.x, y, box.min.z);
                outB = glm::vec3(box.max.z, y, box.max.z);
                break;
        }
    } else if (sliceMode == SliceMode::DiagonalXY) {
        float z = (box.min.z + box.max.z) * 0.5f;
        switch (diagonalCutCorner % 2) {
            case 0:
                outA = glm::vec3(box.min.x, box.max.y, z);
                outB = glm::vec3(box.max.x, box.min.y, z);
                break;
            default:
                outA = glm::vec3(box.min.x, box.min.y, z);
                outB = glm::vec3(box.max.x, box.max.y, z);
                break;
        }
    } else { // DiagonalYZ
        float x = (box.min.x + box.max.x) * 0.5f;
        switch (diagonalCutCorner % 2) {
            case 0:
                outA = glm::vec3(x, box.max.y, box.min.z);
                outB = glm::vec3(x, box.min.y, box.max.z);
                break;
            default:
                outA = glm::vec3(x, box.min.y, box.min.z);
                outB = glm::vec3(x, box.max.y, box.max.z);
                break;
        }
    }
    return true;
}

void SceneEditor::applyDiagonalSlice() {
    if (selected < 0 || selected >= static_cast<int>(objects.size())) return;
    if (sliceMode == SliceMode::Axis) return;
    
    const AABB sourceBox = objects[selected].boundingAABB();
    glm::vec3 color = objects[selected].color();
    
    TriangularPrism prismA, prismB;
    
    if (sliceMode == SliceMode::DiagonalXZ) {
        prismA = TriangularPrism::fromAABBDiagonalXZ(sourceBox, diagonalCutCorner, color);
        prismB = TriangularPrism::fromAABBDiagonalXZ(sourceBox, (diagonalCutCorner + 2) % 4, color);
    } else if (sliceMode == SliceMode::DiagonalXY) {
        prismA = TriangularPrism::fromAABBDiagonalXY(sourceBox, diagonalCutCorner, color);
        prismB = TriangularPrism::fromAABBDiagonalXY(sourceBox, (diagonalCutCorner + 2) % 4, color);
    } else {
        prismA = TriangularPrism::fromAABBDiagonalYZ(sourceBox, diagonalCutCorner, color);
        prismB = TriangularPrism::fromAABBDiagonalYZ(sourceBox, (diagonalCutCorner + 2) % 4, color);
    }
    
    pushSnapshot();
    int oldSelected = selected;
    objects.push_back(SceneObject::fromPrism(prismA));
    objects.push_back(SceneObject::fromPrism(prismB));
    eraseAndRemap(oldSelected);
    
    Log::info("Diagonal slice applied — " +
              std::string(sliceMode == SliceMode::DiagonalXZ ? "XZ" :
                          sliceMode == SliceMode::DiagonalXY ? "XY" : "YZ") +
              " corner " + std::to_string(diagonalCutCorner));
    
    sliceMode = SliceMode::Axis;
    sliceActive = false;
    sliceAxis = -1;
    tool = Tool::Select;
}

void SceneEditor::addMesh(const Mesh& meshData, const glm::vec3& position)
{
    Log::info("addMesh called with " + std::to_string(meshData.vertices.size()) + " vertices");

    if (meshData.vertices.empty()) {
        Log::warn("Cannot add empty mesh to scene");
        return;
    }

    try {
        const size_t vertexCount = meshData.vertices.size();

        Log::info("Creating proxy for large mesh (" + std::to_string(vertexCount) + " verts)");

        // Direct in-place construction - no temporary SceneObject at all
        objects.emplace_back();
        SceneObject& obj = objects.back();

        obj.type = ShapeType::Box;

        // Generous fixed AABB (no vertex iteration, no copy issues)
        obj.box.min = position - glm::vec3(50.0f, 50.0f, 50.0f);
        obj.box.max = position + glm::vec3(50.0f, 50.0f, 50.0f);
        obj.box.color = glm::vec3(0.7f, 0.5f, 0.9f);   // purple for loaded meshes

        int newIndex = static_cast<int>(objects.size()) - 1;

        clearSelection();
        selected = newIndex;
        multiSelected.insert(newIndex);

        spatialDirty = true;

        Log::info("Mesh proxy successfully added at index " + std::to_string(newIndex));

    } catch (const std::exception& e) {
        Log::error("Exception in addMesh: " + std::string(e.what()));
        throw;
    } catch (...) {
        Log::error("Unknown exception in addMesh");
        throw std::runtime_error("vector");
    }
}
