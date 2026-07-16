#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

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

        const std::shared_ptr<class VertexArray>& GetVertexArray() const { return m_VertexArray; }
        int GetIndexCount() const { return m_IndexCount; }

        static std::shared_ptr<Mesh> CreateCube();

    private:
        std::shared_ptr<class VertexArray> m_VertexArray;
        int m_IndexCount;

        void SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    };

} // namespace VECTOR
