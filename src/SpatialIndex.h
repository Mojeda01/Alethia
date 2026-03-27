#pragma once

#include "AABB.h"
#include "SceneObject.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>

class SpatialIndex{
public:
    explicit SpatialIndex(float cellSize = 1.0f);
    
    void clear();
    void insert(int objectId, const AABB& aabb);
    void rebuild(const std::vector<SceneObject>& objects); // full rebuild
    
    // Query functions
    std::vector<int> queryPoint(const glm::vec3& point) const;
    std::vector<int> queryRay(const glm::vec3& origin,
                              const glm::vec3& dir,
                              float maxDist = 1000.0f) const;
    std::vector<int> queryAABB(const AABB& queryBox) const;
    
    void setCellSize(float newSize);
private:
    float cellSize;
    std::unordered_map<uint64_t, std::vector<int>> grid;
    
    uint64_t hash(int x, int z) const;
    void getCellRange(const AABB& aabb, int& minX, int& maxX, int& minZ, int& maxZ) const;
};
