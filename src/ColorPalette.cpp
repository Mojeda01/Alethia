#include "ColorPalette.h"
#include <imgui.h>

ColorPalette::ColorPalette() {
    entries = {
        { "Default",  glm::vec3(1.00f, 1.00f, 1.00f) },
        { "Wall",     glm::vec3(0.75f, 0.73f, 0.68f) },
        { "Floor",    glm::vec3(0.55f, 0.50f, 0.42f) },
        { "Ceiling",  glm::vec3(0.85f, 0.83f, 0.78f) },
        { "Wood",     glm::vec3(0.60f, 0.40f, 0.22f) },
        { "Stone",    glm::vec3(0.50f, 0.48f, 0.45f) },
        { "Water",    glm::vec3(0.20f, 0.45f, 0.70f) },
        { "Exterior", glm::vec3(0.42f, 0.52f, 0.38f) },
    };
}

void ColorPalette::addEntry(const std::string& name, const glm::vec3& color)
{
    entries.push_back({ name, color });
}

void ColorPalette::drawUI(){
    justSelected = false;

    ImGui::Text("Material Category");
    ImGui::Separator();

    for (int i = 0; i < static_cast<int>(entries.size()); ++i){
        const auto& e = entries[i];

        // colored square swatch 
        ImVec4 swatch(e.color.r, e.color.g, e.color.b, 1.0f);
        ImGui::ColorButton(("##swatch" + std::to_string(i)).c_str(),
                            swatch,
                            ImGuiColorEditFlags_NoTooltip |
                            ImGuiColorEditFlags_NoDragDrop,
                            ImVec2(16, 16));
        ImGui::SameLine();

        bool isActive = (i == activeIndex);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button,
                    ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
        }

        if (ImGui::Button(e.name.c_str(), ImVec2(120, 0))) {
            activeIndex  = i;
            justSelected = true;
        }

        if (isActive) {
            ImGui::PopStyleColor();
        }
    }
    ImGui::Separator();
}
