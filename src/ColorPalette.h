#pragma once 

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct PaletteEntry{
    std::string name;
    glm::vec3 color;
};

class ColorPalette{
public:
    ColorPalette();

    void drawUI();

    const glm::vec3& activeColor() const { return entries[activeIndex].color; }
    bool colorWasJustSelected() const { return justSelected; }
    void clearJustSelected() { justSelected = false; }

    void addEntry(const std::string& name, const glm::vec3& color);
private:
    std::vector<PaletteEntry> entries;
    int activeIndex = 0;
    bool justSelected = false;
};
