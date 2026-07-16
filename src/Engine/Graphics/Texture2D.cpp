#include "Engine/Graphics/Texture2D.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLTexture2D.hpp"
#include "Engine/Graphics/DirectX/DirectX12Texture2D.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    std::shared_ptr<Texture2D> Texture2D::Create(const std::string& path) {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLTexture2D>(path);
            case RendererAPI::API::DirectX12:
                return std::make_shared<DirectX12Texture2D>(path);
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

    std::shared_ptr<Texture2D> Texture2D::CreateFromPixels(const void* pixels, int width, int height) {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLTexture2D>(pixels, width, height);
            case RendererAPI::API::DirectX12:
                return std::make_shared<DirectX12Texture2D>(pixels, width, height);
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR

