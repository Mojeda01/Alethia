#include "DebugUI.h"

#include <imgui.h>

void DebugUI::addPanel(const std::string& name, std::function<void()> drawFn) {
    panels.push_back({ name, std::move(drawFn), true });
}

void DebugUI::draw() {
    if (!visible) return; 
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 200), ImVec2(600, 2000));
    ImGui::Begin("Alethia Debug", &visible, ImGuiWindowFlags_NoCollapse);

    for (auto& panel : panels) {
        if (ImGui::CollapsingHeader(panel.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) { 
            panel.drawFn();
        }
    }
    ImGui::End();
}
