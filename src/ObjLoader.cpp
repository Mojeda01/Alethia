#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"
#include "ObjLoader.h"
#include "Mesh.h"
#include "Log.h"

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

Mesh loadObj(const std::string& objPath)
{
    try {
        tinyobj::ObjReaderConfig reader_config;
        reader_config.triangulate = true;
        reader_config.vertex_color = false;

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(objPath, reader_config)) {
            std::string err = reader.Error();
            if (err.empty()) err = "Unknown parse error";
            Log::error("tinyobj ParseFromFile failed: " + err);
            throw std::runtime_error(err);
        }

        if (!reader.Warning().empty()) {
            Log::warn("OBJ warning: " + reader.Warning());
        }

        const auto& attrib = reader.GetAttrib();
        const auto& shapes = reader.GetShapes();

        Log::info("OBJ parsed - vertices: " + std::to_string(attrib.vertices.size()/3)
                  + ", shapes: " + std::to_string(shapes.size()));

        Mesh mesh;
        mesh.vertices.reserve(attrib.vertices.size() / 3);

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex v{};

                // Position
                int vi = index.vertex_index * 3;
                v.position[0] = attrib.vertices[vi];
                v.position[1] = attrib.vertices[vi + 1];
                v.position[2] = attrib.vertices[vi + 2];

                // Normal
                if (index.normal_index >= 0) {
                    int ni = index.normal_index * 3;
                    v.normal[0] = attrib.normals[ni];
                    v.normal[1] = attrib.normals[ni + 1];
                    v.normal[2] = attrib.normals[ni + 2];
                } else {
                    v.normal[0] = 0.0f;
                    v.normal[1] = 1.0f;
                    v.normal[2] = 0.0f;
                }

                // Texcoord (flip Y for Vulkan)
                if (index.texcoord_index >= 0) {
                    int ti = index.texcoord_index * 2;
                    v.texCoord[0] = attrib.texcoords[ti];
                    v.texCoord[1] = 1.0f - attrib.texcoords[ti + 1];
                } else {
                    v.texCoord[0] = 0.0f;
                    v.texCoord[1] = 0.0f;
                }

                v.color[0] = 0.8f;
                v.color[1] = 0.8f;
                v.color[2] = 0.8f;

                mesh.vertices.push_back(v);
                mesh.indices.push_back(static_cast<uint32_t>(mesh.vertices.size() - 1));
            }
        }

        Log::info("Mesh built successfully: " + std::to_string(mesh.vertices.size())
                  + " vertices, " + std::to_string(mesh.indices.size()) + " indices");

        return mesh;

    } catch (const std::exception& e) {
        Log::error("loadObj failed for " + objPath + ": " + e.what());
        std::cerr << "loadObj EXCEPTION: " << e.what() << std::endl;
        throw;
    } catch (...) {
        Log::error("loadObj failed for " + objPath + ": unknown exception");
        std::cerr << "loadObj UNKNOWN EXCEPTION" << std::endl;
        throw std::runtime_error("vector");
    }
}
