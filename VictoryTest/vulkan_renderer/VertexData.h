#pragma once

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/gtx/hash.hpp>

struct VertexData
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const VertexData& other) const {
        return position == other.position && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<VertexData> {
        size_t operator()(VertexData const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}
