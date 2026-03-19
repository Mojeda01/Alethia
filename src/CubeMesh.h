#pragma once 

#include "Vertex.h"
#include <vector>
#include <cstdint>

struct CubeGeometry{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

CubeGeometry makeCube(float size = 1.0f);
