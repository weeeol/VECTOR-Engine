#include "Engine/Graphics/UniformBufferObject.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLUniformBuffer.hpp"
#include "Engine/Graphics/Vulkan/VulkanUniformBuffer.hpp"
#include "Engine/Core/Logger.hpp"
#include <GL/glew.h>

namespace VECTOR {

    void UniformBuffer::BindShaderBlock(uint32_t shaderProgramID, const char* blockName, uint32_t bindingPoint) {
        if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) {
            GLuint blockIndex = glGetUniformBlockIndex(shaderProgramID, blockName);
            if (blockIndex != GL_INVALID_INDEX) {
                glUniformBlockBinding(shaderProgramID, blockIndex, bindingPoint);
            } else {
                VECTOR_LOG_WARN(std::string("Shader block not found: ") + blockName + " in shader program " + std::to_string(shaderProgramID));
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
            case RendererAPI::API::Vulkan:
                return std::make_unique<VulkanUniformBuffer>(size, bindingPoint);
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
