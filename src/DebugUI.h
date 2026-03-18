#pragma once 

#include <functional>
#include <string>
#include <vector>

class DebugUI{
public:
    DebugUI() = default;

    DebugUI(const DebugUI&) = delete;
    DebugUI& operator=(const DebugUI&) = delete;
    DebugUI(DebugUI&&) = default;
    DebugUI& operator=(DebugUI&&) = default;

    void addPanel(const std::string& name, std::function<void()> drawFn);
    void draw();
    bool visible = true;
private:
    struct Panel{
        std::string name;
        std::function<void()> drawFn;
        bool open = true;
    };
    std::vector<Panel> panels;
};
