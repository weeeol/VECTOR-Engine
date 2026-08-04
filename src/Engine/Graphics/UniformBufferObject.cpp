#include "Engine/Graphics/UniformBufferObject.hpp"
#include "Engine/Graphics/RendererAPI.hpp"

#include "Engine/Graphics/Vulkan/VulkanUniformBuffer.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    std::unique_ptr<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t bindingPoint) {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;

            case RendererAPI::API::Vulkan:
                return std::make_unique<VulkanUniformBuffer>(size, bindingPoint);
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
