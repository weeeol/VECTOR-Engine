#include "Engine/Graphics/Buffer.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLBuffer.hpp"
#include "Engine/Graphics/DirectX/DirectX12Buffer.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    VertexBuffer* VertexBuffer::Create(const void* vertices, uint32_t size) {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return new OpenGLVertexBuffer(vertices, size);
            case RendererAPI::API::DirectX12:
                return new DirectX12VertexBuffer(vertices, size);
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

    IndexBuffer* IndexBuffer::Create(const uint32_t* indices, uint32_t count) {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return new OpenGLIndexBuffer(indices, count);
            case RendererAPI::API::DirectX12:
                return new DirectX12IndexBuffer(indices, count);
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
