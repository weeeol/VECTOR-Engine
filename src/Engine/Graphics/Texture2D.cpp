#include "Engine/Graphics/Texture2D.hpp"
#include "Engine/Graphics/RendererAPI.hpp"

#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/Vulkan/VulkanTexture2D.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Texture2D.hpp"

namespace VECTOR {

std::shared_ptr<Texture2D> Texture2D::Create(const std::string &path) {
  switch (RendererAPI::GetAPI()) {
  case RendererAPI::API::None:
    VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
    return nullptr;

  case RendererAPI::API::Vulkan:
    return std::make_shared<VulkanTexture2D>(path);

  case RendererAPI::API::DirectX12:
    return std::make_shared<DirectX12Texture2D>(path);
  }

  VECTOR_LOG_ERROR("Unknown RendererAPI!");
  return nullptr;
}

std::shared_ptr<Texture2D> Texture2D::CreateRenderTarget(uint32_t width, uint32_t height, TextureFormat format) {
  switch (RendererAPI::GetAPI()) {
  case RendererAPI::API::None:
    VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
    return nullptr;
  case RendererAPI::API::Vulkan:
    return std::make_shared<VulkanTexture2D>(width, height, format);
  case RendererAPI::API::DirectX12:
    return std::make_shared<DirectX12Texture2D>(width, height, format);
  }
  VECTOR_LOG_ERROR("Unknown RendererAPI!");
  return nullptr;
}

} // namespace VECTOR
