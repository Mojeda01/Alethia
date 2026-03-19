#include "CubeMesh.h"

CubeGeometry makeCube(float size) {
    float h = size * 0.5f;
    CubeGeometry geo;

    struct Face{
        float pos[4][3];
        float nx, ny, nz;
    };

    Face faces[6] = {
        {{ { h, -h,  h}, { h, -h, -h}, { h,  h, -h}, { h,  h,  h} },  1,  0,  0 },
        {{ {-h, -h, -h}, {-h, -h,  h}, {-h,  h,  h}, {-h,  h, -h} }, -1,  0,  0 },
        {{ {-h,  h,  h}, { h,  h,  h}, { h,  h, -h}, {-h,  h, -h} },  0,  1,  0 },
        {{ {-h, -h, -h}, { h, -h, -h}, { h, -h,  h}, {-h, -h,  h} },  0, -1,  0 },
        {{ {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h} },  0,  0,  1 },
        {{ { h, -h, -h}, {-h, -h, -h}, {-h,  h, -h}, { h,  h, -h} },  0,  0, -1 },
    };

    float uvs[4][2] = {
        {0.0f, 1.0f},
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f},
    };

    for (int f = 0; f < 6; ++f) {
        uint32_t base = static_cast<uint32_t>(geo.vertices.size());

        for (int v = 0; v < 4; ++v) {
            Vertex vert{};
            vert.position[0] = faces[f].pos[v][0];
            vert.position[1] = faces[f].pos[v][1];
            vert.position[2] = faces[f].pos[v][2];
            vert.color[0] = 1.0f;
            vert.color[1] = 1.0f;
            vert.color[2] = 1.0f;
            vert.normal[0] = faces[f].nx;
            vert.normal[1] = faces[f].ny;
            vert.normal[2] = faces[f].nz;
            vert.texCoord[0] = uvs[v][0];
            vert.texCoord[1] = uvs[v][1];
            geo.vertices.push_back(vert);
        }
        geo.indices.push_back(base + 0);
        geo.indices.push_back(base + 1);
        geo.indices.push_back(base + 2);
        geo.indices.push_back(base + 0);
        geo.indices.push_back(base + 2);
        geo.indices.push_back(base + 3);
    }
    return geo;
}
