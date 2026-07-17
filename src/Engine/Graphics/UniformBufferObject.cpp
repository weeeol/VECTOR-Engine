#include "Engine/Graphics/UniformBufferObject.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLUniformBuffer.hpp"
#include "Engine/Core/Logger.hpp"
#include <GL/glew.h>

namespace VECTOR {

    void UniformBuffer::BindShaderBlock(uint32_t shaderProgramID, const char* blockName, uint32_t bindingPoint) {
        // This is still using OpenGL directly! Let's handle this in the RendererAPI or OpenGLRenderer?
        // Wait, the plan says abstract UniformBufferObject. 
        // BindShaderBlock takes a shaderProgramID. This is tied to OpenGL.
        // Let's implement it with a check if OpenGL for now.
        if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) {
            GLuint blockIndex = glGetUniformBlockIndex(shaderProgramID, blockName);
            if (blockIndex != GL_INVALID_INDEX) {
                glUniformBlockBinding(shaderProgramID, blockIndex, bindingPoint);
            }
        }
    }

    std::unique_ptr<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t bindingPoint) {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_unique<OpenGLUniformBuffer>(size, bindingPoint);
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
