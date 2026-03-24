#pragma once

#include "TriangularPrism.h"
#include "Vertex.h"
#include <vector>
#include <cstdint>

struct PrismGeometry {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
};

PrismGeometry makePrismMesh(const TriangularPrism& prism);
