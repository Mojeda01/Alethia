#pragma once 

/* Defines the verte layout - position + color as vec3 each with Vulkan
 * binding/attribute descriptions. */

#include <vulkan/vulkan.h>
#include <array>

struct Vertex{
    float position[3];
    float color[3];

    static VkVertexInputBindingDescription bindingDescription() {
        VkVertexInputBindingDescription desc{};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }
    
    static std::array<VkVertexInputAttributeDescription, 2> attributeDescription() {
        std::array<VkVertexInputAttributeDescription, 2> attrs{};

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, position);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, color);
    }

};
