#include "Grid.h"
#include "../../VkPipeline.h"
#include <stdexcept>
#include <string>
#include <iostream>

Grid createGrid(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
{
    Grid grid;
    
    grid.pipeline = createFullScreenPipeline(device, colorFormat, depthFormat,
        ALETHIA_V2_SHADER_DIR "/level_editor/grid/grid.vert.spv",
        ALETHIA_V2_SHADER_DIR "/level_editor/grid/grid.frag.spv");

    return grid;
}

void drawGrid(VkCommandBuffer cmd, const Grid& grid)
{
    if (!grid.showGrid || grid.pipeline.handle == VK_NULL_HANDLE) {
        std::cout << "[Grid] Skipped (disabled or invalid pipeline)\n";
        return;
    }

    std::cout << "[Grid] Binding grid pipeline...\n";   // ← debug line

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, grid.pipeline.handle);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

void destroyGrid(VkDevice device, Grid& grid)
{
    destroyPipeline(device, grid.pipeline);
}
