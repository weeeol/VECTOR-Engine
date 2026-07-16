#include "Engine/Graphics/OpenGL/OpenGLVertexArray.hpp"
#include <GL/glew.h>
#include <stddef.h>
#include "Engine/Core/Logger.hpp"

// We are assuming a specific vertex layout here for now (Position, Normal, TexCoords)
// In a full implementation, you'd want a BufferLayout class to define this dynamically.
struct Vertex {
    float Position[3];
    float Normal[3];
    float TexCoords[2];
};

namespace VECTOR {

    OpenGLVertexArray::OpenGLVertexArray() {
        glGenVertexArrays(1, &m_RendererID);
    }

    OpenGLVertexArray::~OpenGLVertexArray() {
        glDeleteVertexArrays(1, &m_RendererID);
    }

    void OpenGLVertexArray::Bind() const {
        glBindVertexArray(m_RendererID);
    }

    void OpenGLVertexArray::Unbind() const {
        glBindVertexArray(0);
    }

    void OpenGLVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer) {
        glBindVertexArray(m_RendererID);
        vertexBuffer->Bind();

        if (vertexBuffer->GetLayoutType() == BufferLayoutType::Quad2D) {
            // Pos (2), TexCoord (2)
            // Vertex Positions
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            
            // Vertex Texture Coords
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        } else {
            // Default Mesh3D: Pos(3), Normal(3), TexCoords(2)
            // Vertex Positions
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            
            // Vertex Normals
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
            
            // Vertex Texture Coords
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        }

        m_VertexBuffers.push_back(vertexBuffer);
        glBindVertexArray(0);
    }

    void OpenGLVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) {
        glBindVertexArray(m_RendererID);
        indexBuffer->Bind();

        m_IndexBuffer = indexBuffer;
        glBindVertexArray(0);
    }

} // namespace VECTOR
