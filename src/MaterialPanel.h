#pragma once 

#include "ColorPalette.h"

class MaterialPanel{
public:
    MaterialPanel();
    void draw();
    bool colorWasJustSelected() const;
    const glm::vec3& selectedColor() const;
    void clearPendingSelection();
    bool visible = false;
private:
    ColorPalette palette;
};
