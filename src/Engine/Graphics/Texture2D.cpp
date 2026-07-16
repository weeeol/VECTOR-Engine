#include "Texture2D.hpp"
#include "RendererAPI.hpp"
#include "OpenGL/OpenGLTexture2D.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    std::shared_ptr<Texture2D> Texture2D::Create(const std::string& path) {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLTexture2D>(path);
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
