#include "Engine/Graphics/OpenGL/OpenGLMesh.hpp"
#include <glm/glm.hpp>

namespace VECTOR {

    OpenGLMesh::OpenGLMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
        SetupMesh(vertices, indices);
    }

    OpenGLMesh::~OpenGLMesh() {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
    }

    void OpenGLMesh::SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
        m_IndexCount = static_cast<int>(indices.size());

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        
        // Vertex Normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        
        // Vertex Texture Coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);

        // Calculate AABB
        if (!vertices.empty()) {
            glm::vec3 minAABB = vertices[0].Position;
            glm::vec3 maxAABB = vertices[0].Position;
            for (const auto& v : vertices) {
                minAABB = glm::min(minAABB, v.Position);
                maxAABB = glm::max(maxAABB, v.Position);
            }
            m_AABB.center = (minAABB + maxAABB) * 0.5f;
            m_AABB.extents = (maxAABB - minAABB) * 0.5f;
        }
    }

    void OpenGLMesh::Draw() const {
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

} // namespace VECTOR
