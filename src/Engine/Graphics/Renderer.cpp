#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/RendererAPI.hpp"

#include "Engine/Graphics/Vulkan/VulkanRenderer.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Renderer.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    std::unique_ptr<Renderer> Renderer::Create() {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;

            case RendererAPI::API::Vulkan:
                return std::make_unique<VulkanRenderer>();
            case RendererAPI::API::DirectX12:
                return std::make_unique<DirectX12Renderer>();
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
