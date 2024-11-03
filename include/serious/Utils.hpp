#pragma once
// #include "serious/io/log.hpp"
// #include "serious/graphics/vulkan/Vertex.hpp"

// #define TINYOBJLOADER_IMPLEMENTATION
// #include <tiny_obj_loader.h>

#include <memory>

namespace serious
{

template <class T>
using Ref = std::shared_ptr<T>;

template <class T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// TODO: complete this later
// static void LoadObj(const std::string& path)
// {
//     std::vector<Vertex> vertices;
//     std::vector<uint32_t> indices;

//     tinyobj::ObjReaderConfig reader_config;
//     tinyobj::ObjReader reader;
//     if (!reader.ParseFromFile(path.c_str(), reader_config)) {
//         if (!reader.Error().empty()) {
//             SEError("TinyObjReader error: {}", reader.Error());
//         }
//         exit(1);
//     }

//     auto& attrib = reader.GetAttrib();
//     auto& shapes = reader.GetShapes();
//     for (size_t s = 0; s < shapes.size(); ++s) {
//         size_t index_offset = 0;
//         for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
//             size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
//             for (size_t v = 0; v < fv; v++) {
//                 Vertex vtx {};
//                 tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
//                 tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
//                 tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
//                 tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];
//                 vtx.position = {vx, vy, vz};
//                 if (idx.normal_index >= 0) {
//                     tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
//                     tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
//                     tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
//                     vtx.normal = {nx, ny, nz};
//                 }
//                 if (idx.texcoord_index >= 0) {
//                     tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
//                     tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
//                     vtx.texCoord = {tx, ty};
//                 }
//                 vertices.push_back(vtx);
//                 indices.push_back(uint32_t(index_offset + v));
//             }
//             index_offset += fv;
//         }
//     }
// }

}
