#include "Engine/Graphics/Mesh.hpp"
#include <GL/glew.h>

namespace VECTOR {

    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
        SetupMesh(vertices, indices);
    }

    Mesh::~Mesh() {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
    }

    void Mesh::SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
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

    void Mesh::Draw() const {
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    std::shared_ptr<Mesh> Mesh::CreateCube() {
        std::vector<Vertex> vertices = {
            // Front Face
            {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec2(0.0f, 0.0f)},
            {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec2(1.0f, 0.0f)},
            {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec2(1.0f, 1.0f)},
            {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec2(0.0f, 1.0f)},

            // Back Face
            {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec2(1.0f, 0.0f)},
            {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec2(0.0f, 0.0f)},
            {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec2(0.0f, 1.0f)},
            {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec2(1.0f, 1.0f)},

            // Left Face
            {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 1.0f)},
            {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 1.0f)},
            {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 0.0f)},
            {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 0.0f)},

            // Right Face
            {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 1.0f)},
            {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 1.0f)},
            {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 0.0f)},
            {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 0.0f)},

            // Bottom Face
            {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec2(0.0f, 1.0f)},
            {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec2(1.0f, 1.0f)},
            {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec2(1.0f, 0.0f)},
            {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec2(0.0f, 0.0f)},

            // Top Face
            {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec2(0.0f, 1.0f)},
            {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec2(1.0f, 1.0f)},
            {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec2(1.0f, 0.0f)},
            {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec2(0.0f, 0.0f)}
        };

        std::vector<unsigned int> indices = {
            0,  1,  2,      2,  3,  0,  // Front
            4,  5,  6,      6,  7,  4,  // Back
            8,  9,  10,     10, 11, 8,  // Left
            12, 13, 14,     14, 15, 12, // Right
            16, 17, 18,     18, 19, 16, // Bottom
            20, 21, 22,     22, 23, 20  // Top
        };

        return std::make_shared<Mesh>(vertices, indices);
    }

} // namespace VECTOR
