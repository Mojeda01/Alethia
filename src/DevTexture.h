#pragma once 

#include "TextureImage.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

class DevTexture{
public:
    DevTexture( VkDevice device,
                VkPhysicalDevice physicalDevice,
                VkCommandPool commandPool,
                VkQueue transferQueue,
                int checkerSize = 8,
                int resolution = 256);

    VkImageView view() const { return texture.view(); }
    VkSampler sampler() const { return texture.sampler(); }
private:
    TextureImage texture;
};
