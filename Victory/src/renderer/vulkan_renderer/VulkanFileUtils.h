#include <string>

#include "VertexData.h"

// Load Object
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// Load Image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Victory 
{
    static void LoadModel(const std::string& path_, std::vector<VertexData>& vertices_, std::vector<uint16_t>& indices_)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path_.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<VertexData, uint32_t> uniqueVertices;
        uniqueVertices.reserve(attrib.vertices.size());

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                VertexData vertex{};

                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.f, 1.f, 1.f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices_.size());
                    vertices_.emplace_back(vertex);
                }

                indices_.emplace_back(uniqueVertices[vertex]);
            }
        }
    }

    static unsigned char* LoadPixels(const std::string& path_, int& texWidth, int& texHeight)
    {
        int texChannels;
        stbi_uc* pixels = stbi_load(path_.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels) {
            throw std::runtime_error("Textrue was not loaded");
        }

        return pixels;
    }

    static void DeletePixels(unsigned char* pixels)
    {
        stbi_image_free(pixels);
    }

}