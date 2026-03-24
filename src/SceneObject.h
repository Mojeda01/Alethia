#pragma once

#include "AABB.h"
#include "TriangularPrism.h"
#include <glm/glm.hpp>

enum class ShapeType : uint8_t {
    Box,
    Prism
};

struct SceneObject {
    ShapeType type  = ShapeType::Box;
    AABB      box{};
    TriangularPrism prism{};

    glm::vec3 color() const {
        return type == ShapeType::Box ? box.color : prism.color;
    }

    void setColor(const glm::vec3& c) {
        if (type == ShapeType::Box) box.color   = c;
        else                        prism.color = c;
    }

    AABB boundingAABB() const {
        return type == ShapeType::Box ? box : prism.getBoundingAABB();
    }

    static SceneObject fromBox(const AABB& b) {
        SceneObject o;
        o.type = ShapeType::Box;
        o.box  = b;
        return o;
    }

    static SceneObject fromPrism(const TriangularPrism& p) {
        SceneObject o;
        o.type  = ShapeType::Prism;
        o.prism = p;
        return o;
    }
};
