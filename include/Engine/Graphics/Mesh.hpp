#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Engine/Graphics/Frustum.hpp"

namespace VECTOR {

    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
    };

    class Mesh {
    public:
        virtual ~Mesh() = default;

        virtual void Draw() const = 0;

        virtual int GetIndexCount() const = 0;
        virtual const AABB& GetAABB() const = 0;

        static std::shared_ptr<Mesh> Create(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
        static std::shared_ptr<Mesh> CreateCube();
    };

} // namespace VECTOR
