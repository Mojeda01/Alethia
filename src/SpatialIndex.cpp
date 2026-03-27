#include "SpatialIndex.h"
#include "SceneObject.h"
#include <cmath>

SpatialIndex::SpatialIndex(float cellSize) : cellSize(cellSize) {}

void SpatialIndex::clear() {
    grid.clear();
}

uint64_t SpatialIndex::hash(int x, int z) const {
    return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(z);
}

void SpatialIndex::getCellRange(const AABB& aabb, int& minX, int& maxX, int& minZ, int& maxZ) const {
    minX = static_cast<int>(std::floor(aabb.min.x / cellSize));
    maxX = static_cast<int>(std::floor(aabb.max.x / cellSize));
    minZ = static_cast<int>(std::floor(aabb.min.z / cellSize));
    maxZ = static_cast<int>(std::floor(aabb.max.z / cellSize));
}

void SpatialIndex::insert(int objectId, const AABB& aabb) {
    int minX, maxX, minZ, maxZ;
    getCellRange(aabb, minX, maxX, minZ, maxZ);
    
    for (int x = minX; x <= maxX; ++x) {
        for (int z = minZ; z <= maxZ; ++z) {
            grid[hash(x, z)].push_back(objectId);
        }
    }
}

void SpatialIndex::rebuild(const std::vector<SceneObject>& objects) {
    clear();
    for (int i = 0; i < static_cast<int>(objects.size()); ++i) {
        insert(i, objects[i].boundingAABB());
    }
}

std::vector<int> SpatialIndex::queryPoint(const glm::vec3& point) const {
    int x = static_cast<int>(std::floor(point.x / cellSize));
    int z = static_cast<int>(std::floor(point.z / cellSize));

        auto it = grid.find(hash(x, z));
        if (it != grid.end()) {
            return it->second;        // return copy of the vector
        }

        return std::vector<int>{};    // return empty vector
}

std::vector<int> SpatialIndex::queryAABB(const AABB& queryBox) const {
    std::vector<int> result;
    int minX, maxX, minZ, maxZ;
    getCellRange(queryBox, minX, maxX, minZ, maxZ);

    for (int x = minX; x <= maxX; ++x) {
        for (int z = minZ; z <= maxZ; ++z) {
            auto it = grid.find(hash(x, z));
            if (it != grid.end()) {
                for (int id : it->second) {
                    result.push_back(id);        // ← Fixed: added 'id'
                }
            }
        }
    }
    return result;
}

void SpatialIndex::setCellSize(float newSize){
    if (newSize > 0.0f) {
        cellSize = newSize;
    }
}
