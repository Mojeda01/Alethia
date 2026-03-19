#include "DevTexture.h"

static std::vector<uint8_t> generateCheckerboard(int resolution, int checkerSize) {
    std::vector<uint8_t> pixels(resolution * resolution * 4);

    for (int y = 0; y < resolution; ++y) {
        for (int x = 0; x < resolution; ++x) {
            int cx = x / checkerSize;
            int cy = y / checkerSize;
            bool white = ((cx + cy) % 2) == 0;

            int idx = (y * resolution + x) * 4;
            uint8_t val = white ? 200 : 40;
            pixels[idx + 0] = val;
            pixels[idx + 1] = val;
            pixels[idx + 2] = val;
            pixels[idx + 3] = 255;
        }
    }
    return pixels;
}

DevTexture::DevTexture(VkDevice device,
                       VkPhysicalDevice physicalDevice,
                       VkCommandPool commandPool,
                       VkQueue transferQueue,
                       int checkerSize,
                       int resolution)
    : texture(device, physicalDevice, commandPool, transferQueue,
              generateCheckerboard(resolution, checkerSize).data(),
              resolution, resolution)
{
}
