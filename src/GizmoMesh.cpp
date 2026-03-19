#include "GizmoMesh.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

static const int SIDES = 16;
static const float SHAFT_RADIUS  = 0.03f;
static const float SHAFT_LENGTH  = 0.5f;
static const float CONE_RADIUS   = 0.07f;
static const float CONE_LENGTH   = 0.2f;

static void appendArrow(
    GizmoGeometry& geo,
    glm::vec3 dir,
    glm::vec3 perp,
    float r, float g, float b
){
    glm::vec3 up = glm::normalize(dir);
    glm::vec3 side = glm::normalize(perp);
    glm::vec3 fwd = glm::cross(up, side);

    uint32_t base = static_cast<uint32_t>(geo.vertices.size());

    auto addVert = [&](glm::vec3 pos) {
        Vertex v{};
        v.position[0] = pos.x;
        v.position[1] = pos.y;
        v.position[2] = pos.z;
        v.color[0] = r;
        v.color[1] = g;
        v.color[2] = b;
        geo.vertices.push_back(v);
    };

    for (int i = 0; i < SIDES; ++i) {
        float a0 = (float)i / SIDES * 2.0f * 3.14159265f;
        float a1 = (float)(i + 1) / SIDES * 2.0f * 3.14159265f;
        
        glm::vec3 n0 = std::cos(a0) + side + std::sin(a0) * fwd;
        glm::vec3 n1 = std::cos(a1) + side + std::sin(a1) * fwd;

        glm::vec3 b0 = n0 * SHAFT_RADIUS;
        glm::vec3 b1 = n1 * SHAFT_RADIUS;
        glm::vec3 t0 = b0 + up * SHAFT_LENGTH;
        glm::vec3 t1 = b1 + up * SHAFT_LENGTH;

        uint32_t vi = base + static_cast<uint32_t>(geo.vertices.size() - base);
        addVert(b0); addVert(b1); addVert(t1); addVert(t0);

        uint32_t idx = static_cast<uint32_t>(geo.vertices.size()) - 4;
        geo.indices.push_back(idx + 0);
        geo.indices.push_back(idx + 1);
        geo.indices.push_back(idx + 2);
        geo.indices.push_back(idx + 0);
        geo.indices.push_back(idx + 2);
        geo.indices.push_back(idx + 3);
    }

    glm::vec3 coneBase = up * SHAFT_LENGTH;
    glm::vec3 coneTip = up * (SHAFT_LENGTH + CONE_LENGTH);

    for (int i = 0; i < SIDES; ++i) {
        float a0 = (float)i / SIDES * 2.0f * 3.14159265f;
        float a1 = (float)(i + 1) / SIDES * 2.0f * 3.14159265f;

        glm::vec3 n0 = std::cos(a0) * side + std::sin(a0) * fwd;
        glm::vec3 n1 = std::cos(a1) * side + std::sin(a1) * fwd;
        glm::vec3 c0 = coneBase + n0 * CONE_RADIUS;
        glm::vec3 c1 = coneBase + n1 * CONE_RADIUS;

        uint32_t idx = static_cast<uint32_t>(geo.vertices.size());
        addVert(c0);
        addVert(c1);
        addVert(coneTip);

        geo.indices.push_back(idx + 0);
        geo.indices.push_back(idx + 1);
        geo.indices.push_back(idx + 2);
    }
}

GizmoGeometry makeGizmoMesh() {
    GizmoGeometry geo;
    appendArrow(geo, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), 1.0f, 0.0f, 0.0f);
    appendArrow(geo, glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), 0.0f, 1.0f, 0.0f);
    appendArrow(geo, glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), 0.0f, 0.0f, 1.0f);
    return geo;
}
