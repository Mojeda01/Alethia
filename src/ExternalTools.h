#pragma once

#include <functional>
#include <string>
#include <vector>

class ExternalTools{
public:
    ExternalTools() = default;
    
    ExternalTools(const ExternalTools&) = delete;
    ExternalTools& operator=(const ExternalTools&) = delete;
    ExternalTools(ExternalTools&&) = default;
    ExternalTools& operator=(ExternalTools&&) = default;
    
    // Core panel system - same pattern as DebugUI
    void addPanel(const std::string& name, std::function<void()> drawFn);
    
    void draw(); // Draws all external panels
    bool& visible() { return m_visible; }
    
    // External tool execution API
    void executeBinaryTool(const std::string& toolName);
    void testExecution();
    
private:
    struct Panel {
            std::string name;
            std::function<void()> drawFn;
            bool open = true;
        };

        std::vector<Panel> m_panels;
        bool m_visible = true;
};
