#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"
#include "ObjLoader.h"
#include "Mesh.h"

#include <stdexcept>
#include <unordered_map>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>

namespace {
struct VertexKey {
    int vi, ni, ti;
    bool operator==(const VertexKey& o) const {
        return vi == o.vi && ni == o.ni && ti == o.ti;
    }
};

struct VertexKeyHash {
    size_t operator()(const VertexKey& k) const {
        size_t h = std::hash<int>()(k.vi);
        h ^= std::hash<int>()(k.ni) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(k.ti) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};
} // namespace

Mesh loadObj(const std::string& objPath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    std::string mtlDir = objPath.substr(0, objPath.find_last_of("/\\") + 1);

    {
        std::string testPath = mtlDir + "Castelia City.mtl";
        std::ifstream testFile(testPath);
        std::cout << "MTL test path: [" << testPath << "] exists: " << testFile.good() << "\n";
    }
 
    std::string mtlPath = mtlDir + "Castelia City.mtl";
    std::map<std::string, int> matMap;
    std::ifstream mtlStream(mtlPath);
    if (mtlStream.good()) {
        tinyobj::LoadMtl(&matMap, &materials, &mtlStream, &warn, &err);
        std::cout << "MTL loaded directly: " << materials.size() << " materials\n";
        mtlStream.close();
    }
    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
            objPath.c_str(), mtlDir.c_str(), true);

    if (!ok) {
        throw std::runtime_error("Failed to load OBJ: " + err);
    }
   
    if (!warn.empty()) {
        std::cout << "OBJ warning: " << warn << "\n";
    }
    if (!err.empty()) {
        std::cout << "OBJ error: " << err << "\n";
    }

    std::cout << "OBJ loaded: " << materials.size() << " materials\n";
    for (size_t i = 0; i < materials.size() && i < 5; ++i) {
        std::cout << "  mat[" << i << "] name=" << materials[i].name
            << " diffuse_texname=" << materials[i].diffuse_texname << "\n";
    }
    
    Mesh mesh;
        
    std::unordered_map<VertexKey, uint32_t, VertexKeyHash> uniqueVerts;

    for (const auto& shape : shapes) {
        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];
            int matId = -1;
            if (f < shape.mesh.material_ids.size()) {
                matId = shape.mesh.material_ids[f];
            }
            float r = 0.6f, g = 0.6f, b = 0.6f;
            if (matId >= 0 && matId < static_cast<int>(materials.size())) {
                r = materials[matId].diffuse[0];
                g = materials[matId].diffuse[1];
                b = materials[matId].diffuse[2];
            }
            for (int v = 1; v < fv - 1; ++v) {
                int triIndices[3] = { 0, v, v + 1 };

                for (int t = 0; t < 3; ++t) {
                    tinyobj::index_t idx = shape.mesh.indices[indexOffset + triIndices[t]];
                    VertexKey key{ idx.vertex_index, idx.normal_index, idx.texcoord_index };
                    auto it = uniqueVerts.find(key);
                    if (it != uniqueVerts.end()) {
                        mesh.indices.push_back(it->second);
                    } else {
                        Vertex vert{};

                        vert.position[0] = attrib.vertices[3 * idx.vertex_index + 0];
                        vert.position[1] = attrib.vertices[3 * idx.vertex_index + 1];
                        vert.position[2] = attrib.vertices[3 * idx.vertex_index + 2];

                        vert.color[0] = r;
                        vert.color[1] = g;
                        vert.color[2] = b;

                        if (idx.normal_index >= 0) {
                            vert.normal[0] = attrib.normals[3 * idx.normal_index + 0];
                            vert.normal[1] = attrib.normals[3 * idx.normal_index + 1];
                            vert.normal[2] = attrib.normals[3 * idx.normal_index + 2];
                        } else {
                            vert.normal[0] = 0.0f;
                            vert.normal[1] = 1.0f;
                            vert.normal[2] = 0.0f;
                        }

                        if (idx.texcoord_index >= 0) {
                            vert.texCoord[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
                            vert.texCoord[1] = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
                        } else {
                            vert.texCoord[0] = 0.0f;
                            vert.texCoord[1] = 0.0f;
                        }

                        uint32_t newIndex = static_cast<uint32_t>(mesh.vertices.size());
                        mesh.vertices.push_back(vert);
                        mesh.indices.push_back(newIndex);
                        uniqueVerts[key] = newIndex;
                    }
                }
            }
            indexOffset += fv;
        }
    }
    if (mesh.vertices.empty()) {
        throw std::runtime_error("OBJ file contains no geometry: " + objPath);
    }
    return mesh;
}
