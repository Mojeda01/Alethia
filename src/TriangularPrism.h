#pragma once

#include "AABB.h"
#include <glm/glm.hpp>

struct TriangularPrism {
    glm::vec3 v0, v1, v2;
    glm::vec3 extrudeDir;
    glm::vec3 color = glm::vec3(1.0f);

    glm::vec3 diagonalNormal() const {
        glm::vec3 edge0 = v1 - v0;
        glm::vec3 edge1 = v2 - v0;
        return glm::normalize(glm::cross(edge0, edge1));
    }

    AABB getBoundingAABB() const {
        glm::vec3 v3 = v0 + extrudeDir;
        glm::vec3 v4 = v1 + extrudeDir;
        glm::vec3 v5 = v2 + extrudeDir;

        glm::vec3 mn = glm::min(glm::min(glm::min(v0, v1),
                                          glm::min(v2, v3)),
                                 glm::min(v4, v5));
        glm::vec3 mx = glm::max(glm::max(glm::max(v0, v1),
                                          glm::max(v2, v3)),
                                 glm::max(v4, v5));
        AABB result;
        result.min   = mn;
        result.max   = mx;
        result.color = color;
        return result;
    }

    // XZ diagonal — triangle in XZ plane, extruded along Y
    static TriangularPrism fromAABBDiagonalXZ(
        const AABB& box, int cutCorner,
        const glm::vec3& color = glm::vec3(1.0f))
    {
        TriangularPrism p;
        p.color       = color;
        p.extrudeDir  = glm::vec3(0.0f, box.max.y - box.min.y, 0.0f);

        float x0 = box.min.x, x1 = box.max.x;
        float z0 = box.min.z, z1 = box.max.z;
        float y  = box.min.y;

        switch (cutCorner) {
            case 0:
                p.v0 = glm::vec3(x1, y, z0);
                p.v1 = glm::vec3(x1, y, z1);
                p.v2 = glm::vec3(x0, y, z1);
                break;
            case 1:
                p.v0 = glm::vec3(x0, y, z0);
                p.v1 = glm::vec3(x0, y, z1);
                p.v2 = glm::vec3(x1, y, z1);
                break;
            case 2:
                p.v0 = glm::vec3(x0, y, z0);
                p.v1 = glm::vec3(x1, y, z0);
                p.v2 = glm::vec3(x0, y, z1);
                break;
            default:
                p.v0 = glm::vec3(x0, y, z0);
                p.v1 = glm::vec3(x1, y, z0);
                p.v2 = glm::vec3(x1, y, z1);
                break;
        }
        return p;
    }

    // XY diagonal — triangle in XY plane, extruded along Z
    static TriangularPrism fromAABBDiagonalXY(
        const AABB& box, int cutCorner,
        const glm::vec3& color = glm::vec3(1.0f))
    {
        TriangularPrism p;
        p.color      = color;
        p.extrudeDir = glm::vec3(0.0f, 0.0f, box.max.z - box.min.z);

        float x0 = box.min.x, x1 = box.max.x;
        float y0 = box.min.y, y1 = box.max.y;
        float z  = box.min.z;

        switch (cutCorner) {
            case 0:
                p.v0 = glm::vec3(x1, y0, z);
                p.v1 = glm::vec3(x1, y1, z);
                p.v2 = glm::vec3(x0, y1, z);
                break;
            case 1:
                p.v0 = glm::vec3(x0, y0, z);
                p.v1 = glm::vec3(x0, y1, z);
                p.v2 = glm::vec3(x1, y1, z);
                break;
            case 2:
                p.v0 = glm::vec3(x0, y0, z);
                p.v1 = glm::vec3(x1, y0, z);
                p.v2 = glm::vec3(x0, y1, z);
                break;
            default:
                p.v0 = glm::vec3(x0, y0, z);
                p.v1 = glm::vec3(x1, y0, z);
                p.v2 = glm::vec3(x1, y1, z);
                break;
        }
        return p;
    }

    // YZ diagonal — triangle in YZ plane, extruded along X
    static TriangularPrism fromAABBDiagonalYZ(
        const AABB& box, int cutCorner,
        const glm::vec3& color = glm::vec3(1.0f))
    {
        TriangularPrism p;
        p.color      = color;
        p.extrudeDir = glm::vec3(box.max.x - box.min.x, 0.0f, 0.0f);

        float y0 = box.min.y, y1 = box.max.y;
        float z0 = box.min.z, z1 = box.max.z;
        float x  = box.min.x;

        switch (cutCorner) {
            case 0:
                p.v0 = glm::vec3(x, y1, z0);
                p.v1 = glm::vec3(x, y1, z1);
                p.v2 = glm::vec3(x, y0, z1);
                break;
            case 1:
                p.v0 = glm::vec3(x, y0, z0);
                p.v1 = glm::vec3(x, y0, z1);
                p.v2 = glm::vec3(x, y1, z1);
                break;
            case 2:
                p.v0 = glm::vec3(x, y0, z0);
                p.v1 = glm::vec3(x, y1, z0);
                p.v2 = glm::vec3(x, y0, z1);
                break;
            default:
                p.v0 = glm::vec3(x, y0, z0);
                p.v1 = glm::vec3(x, y1, z0);
                p.v2 = glm::vec3(x, y1, z1);
                break;
        }
        return p;
    }
};
