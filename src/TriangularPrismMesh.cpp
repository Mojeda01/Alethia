#include "TriangularPrismMesh.h"
#include <glm/gtc/matrix_transform.hpp>

PrismGeometry makePrismMesh(const TriangularPrism& prism) {
    PrismGeometry geo;
    
    // six vertices: v0-v2 on cap A, v3-v5 on cap B
    glm::vec3 capA[3] = { prism.v0, prism.v1, prism.v2 };
    glm::vec3 capB[3] = {
        prism.v0 + prism.extrudeDir,
        prism.v1 + prism.extrudeDir,
        prism.v2 + prism.extrudeDir
    };
    
    auto addFace = [&](glm::vec3 p0, glm::vec3 p1,
                       glm::vec3 p2, glm::vec3 p3,
                       glm::vec3 normal) {
        uint32_t base = static_cast<uint32_t>(geo.vertices.size());
        
        float uvs[4][2] = {
            {0.0f, 1.0f}, {1.0f, 1.0f},
            {1.0f, 0.0f}, {0.0f, 0.0f}
        };
        
        glm::vec3 pts[4] = { p0, p1, p2, p3 };
        for (int i = 0; i < 4; ++i) {
            Vertex v{};
            v.position[0] = pts[i].x;
            v.position[1] = pts[i].y;
            v.position[2] = pts[i].z;
            v.color[0] = 1.0f; v.color[1] = 1.0f; v.color[2] = 1.0f;
            v.normal[0] = normal.x;
            v.normal[1] = normal.y;
            v.normal[2] = normal.z;
            v.texCoord[0] = uvs[i][0];
            v.texCoord[1] = uvs[i][1];
            geo.vertices.push_back(v);
        }
        geo.indices.push_back(base + 0);
        geo.indices.push_back(base + 1);
        geo.indices.push_back(base + 2);
        geo.indices.push_back(base + 0);
        geo.indices.push_back(base + 2);
        geo.indices.push_back(base + 3);
    };
    
    auto addTriangle = [&](glm::vec3 p0, glm::vec3 p1,
                           glm::vec3 p2, glm::vec3 normal) {
        uint32_t base = static_cast<uint32_t>(geo.vertices.size());
        
        float uvs[3][2] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f} };
        glm::vec3 pts[3] = { p0, p1, p2 };
        for (int i = 0; i < 3; ++i) {
            Vertex v{};
            v.position[0] = pts[i].x;
            v.position[1] = pts[i].y;
            v.position[2] = pts[i].z;
            v.color[0] = 1.0f; v.color[1] = 1.0f; v.color[2] = 1.0f;
            v.normal[0] = normal.x;
            v.normal[1] = normal.y;
            v.normal[2] = normal.z;
            v.texCoord[0] = uvs[i][0];
            v.texCoord[1] = uvs[i][1];
            geo.vertices.push_back(v);
        }
        geo.indices.push_back(base + 0);
        geo.indices.push_back(base + 1);
        geo.indices.push_back(base + 2);
    };
    
    glm::vec3 extNorm = glm::normalize(prism.extrudeDir);
    
    // cap A - normal points opposite to extrude direction
    glm::vec3 nA = glm::normalize(glm::cross(capA[1] - capA[0], capA[2] - capA[0]));
    if (glm::dot(nA, extNorm) > 0.0f) nA = -nA;
    addTriangle(capA[0], capA[1], capA[2], nA);
    
    // cap B — normal points along extrude direction
    glm::vec3 nB = -nA;
    addTriangle(capB[0], capB[2], capB[1], nB);
    
    // three quad sides
    for (int i = 0; i < 3; ++i) {
        int j = (i + 1) % 3;
        glm::vec3 edge = capA[j] - capA[i];
        glm::vec3 sideN = glm::normalize(glm::cross(edge, extNorm));
        
        addFace(capA[i], capA[j],
                capB[j], capB[i], sideN);
    }
    return geo;
}
