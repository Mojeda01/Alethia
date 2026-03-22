#pragma once 

#include "AABB.h"
#include <glm/glm.hpp>

struct PhysicsBody{
    glm::vec3 position = glm::vec3(0.0f); // center of body at floor level.
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 size = glm::vec3(0.6f, 1.8f, 0.6f);// width, height, depth in meters

    bool onGround = false;
    bool noclip = false;

    // derives a world-space AABB from current position and size
    // position is the center of the bottom face (feet position)
    AABB getAABB() const {
        return AABB{
            glm::vec3(  position.x - size.x * 0.5f,
                        position.y,
                        position.z - size.z * 0.5f),
            glm::vec3(  position.x + size.x * 0.5f,
                        position.y + size.y,
                        position.z + size.z * 0.5f)
        };
    }
};
