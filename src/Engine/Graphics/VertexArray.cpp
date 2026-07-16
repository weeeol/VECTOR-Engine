#include "Engine/Graphics/VertexArray.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLVertexArray.hpp"
#include "Engine/Graphics/DirectX/DirectX12VertexArray.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    VertexArray* VertexArray::Create() {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return new OpenGLVertexArray();
            case RendererAPI::API::DirectX12:
                return new DirectX12VertexArray();
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
