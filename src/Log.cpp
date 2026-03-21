#include "Log.h"
#include <imgui.h>

static std::vector<Log::Entry> sEntries;
static uint32_t sMaxEntries = 500;
static uint32_t sCurrentFrame = 0;
static bool sAutoScroll = true;
static int sFilterLevel = 0;

void Log::init(uint32_t maxEntries) {
    sMaxEntries = maxEntries;
    sEntries.reserve(maxEntries);
}

void Log::setFrame(uint32_t frame) {
    sCurrentFrame = frame;
}

void Log::info(const std::string& msg) {
    push(Level::Info, msg);
}

void Log::warn(const std::string& msg) {
    push(Level::Warn, msg);
}

void Log::error(const std::string& msg) {
    push(Level::Error, msg);
}

void Log::push(Level level, const std::string& msg) {
    if (sEntries.size() >= sMaxEntries) {
        sEntries.erase(sEntries.begin());
    }
    sEntries.push_back({ level, msg, sCurrentFrame });
}

const std::vector<Log::Entry>& Log::entries() {
    return sEntries;
}

void Log::clear() {
    sEntries.clear();
}

void Log::drawPanel() 
{
    const char* levels[] = { "All", "Warn", "Error" };
    ImGui::Combo("Filter", &sFilterLevel, levels, 3);
    ImGui::SameLine();
    if (ImGui::Button("clear")) {
        clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &sAutoScroll);

    ImGui::Separator();

    ImGui::BeginChild("LogScroll", ImVec2(0, 120), true);

    for (const auto& entry : sEntries) {
        if (sFilterLevel == 1 && entry.level == Level::Info) continue;
        if (sFilterLevel == 2 && entry.level != Level::Error) continue;

        ImVec4 color;
        const char* prefix;
        switch(entry.level) {
            case Level::Info:
                color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                prefix = "[INFO]";
                break;
            case Level::Warn:
                color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
                prefix = "[WARN]";
                break;
            case Level::Error:
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                prefix = "[ERR ]";
                break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("[%u] %s %s", entry.frame, prefix, entry.message.c_str());
        ImGui::PopStyleColor(); 
    }

    if (sAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}
