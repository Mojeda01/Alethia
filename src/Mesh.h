#pragma once
#include "Vertex.h"
#include <vector>
#include <string>

struct Mesh{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string textureName;
};
