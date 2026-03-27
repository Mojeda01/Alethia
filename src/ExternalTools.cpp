#include "ExternalTools.h"

#include <imgui.h>
#include <cstdlib>
#include <thread>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

void ExternalTools::addPanel(const std::string& name, std::function<void()> drawFn)
{
    m_panels.push_back({ name, std::move(drawFn), true });
}

void ExternalTools::draw() {
    if (!m_visible) return;
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(280, 280), ImVec2(800, 2500));
    ImGui::Begin("External Tools", &m_visible, ImGuiWindowFlags_NoCollapse);
    
    for (auto& panel : m_panels) {
        if (ImGui::CollapsingHeader(panel.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            panel.drawFn();
        }
    }
    
    ImGui::End();
}

void ExternalTools::testExecution() {
    const char* path = "src/ExternalPrograms/TestBinaryTool";

        ImGui::Text("Launching: %s", path);

        std::thread([path]() {
            std::system(path);
        }).detach();
}


void ExternalTools::executeBinaryTool(const std::string& toolName) {
    // Future: actual execution logic
    // Could use std::system(), fork/exec, or a proper plugin system
}
