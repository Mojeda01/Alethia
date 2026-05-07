#pragma once

#include <vulkan/vulkan.h>
#include "../../VkPipeline.h"

struct Grid {
    Pipeline pipeline{};
    bool showGrid = true;
};

Grid createGrid(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);
void drawGrid(VkCommandBuffer cmd, const Grid& grid);
void destroyGrid(VkDevice device, Grid& grid);
