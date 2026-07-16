#include "Engine/Graphics/VertexArray.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLVertexArray.hpp"
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
                // TODO: Implement DirectX12VertexArray if applicable
                // DirectX 12 doesn't use VAOs directly in the same way (uses PSOs + Views), 
                // but we can create an abstraction that maps well to it.
                VECTOR_LOG_ERROR("RendererAPI::DirectX12 VertexArray not fully implemented yet!");
                return nullptr;
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
