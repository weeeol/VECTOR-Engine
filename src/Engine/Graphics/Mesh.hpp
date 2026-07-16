#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Frustum.hpp"

namespace VECTOR {

    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
    };

    class Mesh {
    public:
        Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
        ~Mesh();

        void Draw() const;

        unsigned int GetVAO() const { return m_VAO; }
        int GetIndexCount() const { return m_IndexCount; }

        static std::shared_ptr<Mesh> CreateCube();

        const AABB& GetAABB() const { return m_AABB; }

    private:
        unsigned int m_VAO, m_VBO, m_EBO;
        int m_IndexCount;
        AABB m_AABB;

        void SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    };

} // namespace VECTOR
