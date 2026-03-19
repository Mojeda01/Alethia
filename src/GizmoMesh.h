#pragma once

#include "Vertex.h"
#include <vector>

struct GizmoGeometry{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

GizmoGeometry makeGizmoMesh();
