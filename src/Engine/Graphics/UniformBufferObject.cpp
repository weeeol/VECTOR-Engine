#include "UniformBufferObject.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    UniformBuffer::UniformBuffer(uint32_t size, uint32_t bindingPoint)
        : m_BindingPoint(bindingPoint)
    {
        glGenBuffers(1, &m_UBO);
        glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, m_UBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    UniformBuffer::~UniformBuffer() {
        if (m_UBO) {
            glDeleteBuffers(1, &m_UBO);
        }
    }

    void UniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
        glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
        glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void UniformBuffer::Bind() const {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, m_UBO);
    }

    void UniformBuffer::Unbind() const {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, 0);
    }

    void UniformBuffer::BindShaderBlock(uint32_t shaderProgramID, const char* blockName, uint32_t bindingPoint) {
        GLuint blockIndex = glGetUniformBlockIndex(shaderProgramID, blockName);
        if (blockIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(shaderProgramID, blockIndex, bindingPoint);
        }
    }

} // namespace VECTOR
