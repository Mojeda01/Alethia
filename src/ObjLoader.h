#pragma once

#include "Vertex.h"
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

struct Mesh;
Mesh loadObj(const std::string& objPath);
