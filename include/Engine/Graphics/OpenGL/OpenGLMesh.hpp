#pragma once

#include "Engine/Graphics/Mesh.hpp"
#include <vector>
#include <GL/glew.h>

namespace VECTOR {

    class OpenGLMesh : public Mesh {
    public:
        OpenGLMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
        ~OpenGLMesh() override;

        void Draw() const override;

        int GetIndexCount() const override { return m_IndexCount; }
        const AABB& GetAABB() const override { return m_AABB; }

    private:
        unsigned int m_VAO, m_VBO, m_EBO;
        int m_IndexCount;
        AABB m_AABB;

        void SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    };

} // namespace VECTOR
