#include "Engine/Graphics/Cubemap.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/Vulkan/VulkanCubemap.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Cubemap.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    std::shared_ptr<Cubemap> Cubemap::Create(const std::vector<std::string>& faces) {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::Vulkan:
                return std::make_shared<VulkanCubemap>(faces);
            case RendererAPI::API::DirectX12:
                return std::make_shared<DirectX12Cubemap>(faces);
            default:
                VECTOR_LOG_ERROR("Unknown RendererAPI!");
                return nullptr;
        }
    }

} // namespace VECTOR
