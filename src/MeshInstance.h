#pragma once
#include <glm/glm.hpp>

struct MeshInstance{
    uint32_t meshId = 0;
    glm::mat4 transform = glm::mat4(1.0f);
    glm::vec3 color = glm::vec3(1.0f);
};
