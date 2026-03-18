#include "DebugUI.h"

#include <imgui.h>

void DebugUI::addPanel(const std::string& name, std::function<void()> drawFn) {
    panels.push_back({ name, std::move(drawFn), true });
}

void DebugUI::draw() {
    if (!visible) return;

    ImGui::Begin("Alethia Debug");

    for (auto& panel : panels) {
        if (ImGui::CollapsingHeader(panel.name.c_str(), &panel.open, ImGuiTreeNodeFlags_DefaultOpen)) {
            panel.drawFn();
        }
    }
    ImGui::End();
}
