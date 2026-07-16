#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLRendererAPI.hpp"
#include "Engine/Graphics/DirectX/DirectX12RendererAPI.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;

    RendererAPI* RendererAPI::Create() {
        switch (s_API) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return new OpenGLRendererAPI();
            case RendererAPI::API::DirectX12:
                return new DirectX12RendererAPI();
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
