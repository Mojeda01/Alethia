#pragma once

#include "Vertex.h"
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

struct ObjMesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string textureBasePath;
    std::unordered_map<std::string, std::string> materialTextures;
};

ObjMesh loadObj(const std::string& objPath);
