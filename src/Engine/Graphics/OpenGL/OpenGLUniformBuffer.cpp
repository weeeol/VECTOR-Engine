#include "Engine/Graphics/OpenGL/OpenGLUniformBuffer.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t bindingPoint)
        : m_BindingPoint(bindingPoint)
    {
        glGenBuffers(1, &m_UBO);
        glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, m_UBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    OpenGLUniformBuffer::~OpenGLUniformBuffer() {
        if (m_UBO) {
            glDeleteBuffers(1, &m_UBO);
        }
    }

    void OpenGLUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
        glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
        glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void OpenGLUniformBuffer::Bind() const {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, m_UBO);
    }

    void OpenGLUniformBuffer::Unbind() const {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, 0);
    }

} // namespace VECTOR
