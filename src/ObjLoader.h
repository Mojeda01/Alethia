#pragma once

#include "Vertex.h"
#include <vector>
#include <string>
#include <cstdint>

struct ObjMesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

ObjMesh loadObj(const std::string& objPath);
