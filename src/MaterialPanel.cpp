#include "MaterialPanel.h"
#include <imgui.h>

MaterialPanel::MaterialPanel()
    : palette()
{}

void MaterialPanel::draw() {
    if (!visible) return;

    ImGui::SetNextWindowSize(ImVec2(200,300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Materials", &visible, ImGuiWindowFlags_NoCollapse);
    palette.drawUI();
    ImGui::End();
}

bool MaterialPanel::colorWasJustSelected() const {
    return palette.colorWasJustSelected();
}

const glm::vec3& MaterialPanel::selectedColor() const {
    return palette.activeColor();   
}

void MaterialPanel::clearPendingSelection() {
    palette.clearJustSelected();
}
