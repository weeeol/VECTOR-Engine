#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLRenderer.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    std::unique_ptr<Renderer> Renderer::Create() {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_unique<OpenGLRenderer>();
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
