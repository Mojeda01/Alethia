#pragma once

#include <glm/glm.hpp>

struct AABB{
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 color = glm::vec3(1.0f); // white deafult - multiplies with texture.

    glm::vec3 center() const { return (min + max) * 0.5f; }
    glm::vec3 size() const { return max - min; }

    static AABB fromCenterSize(const glm::vec3& center, const glm::vec3& size) {
        glm::vec3 half = size * 0.5f;
        return { center - half, center + half };
    }

    static AABB unitCubeAt(const glm::vec3& snappedPos, float cubeSize) {
        glm::vec3 half(cubeSize * 0.5f, 0.0f, cubeSize * 0.5f);
        AABB result;
        result.min = glm::vec3(snappedPos.x - half.x, snappedPos.y, snappedPos.z - half.z);
        result.max = glm::vec3(snappedPos.x + half.x, snappedPos.y + cubeSize, snappedPos.z + half.z);
        return result;
    }
};
