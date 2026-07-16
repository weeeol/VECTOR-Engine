#include "Engine/Graphics/DirectX/DirectX12Buffer.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    // --- DirectX12VertexBuffer ---
    DirectX12VertexBuffer::DirectX12VertexBuffer(const void* vertices, uint32_t size) {
        VECTOR_LOG_INFO("Creating DirectX12VertexBuffer (Stub)");
        // TODO: Implement actual resource creation, upload heap, default heap, and copy command
    }

    DirectX12VertexBuffer::~DirectX12VertexBuffer() {
    }

    void DirectX12VertexBuffer::Bind() const {
        // Will be called when we have a CommandList abstraction
    }

    void DirectX12VertexBuffer::Unbind() const {
    }

    // --- DirectX12IndexBuffer ---
    DirectX12IndexBuffer::DirectX12IndexBuffer(const uint32_t* indices, uint32_t count)
        : m_Count(count) {
        VECTOR_LOG_INFO("Creating DirectX12IndexBuffer (Stub)");
        // TODO: Implement actual resource creation, upload heap, default heap, and copy command
    }

    DirectX12IndexBuffer::~DirectX12IndexBuffer() {
    }

    void DirectX12IndexBuffer::Bind() const {
        // Will be called when we have a CommandList abstraction
    }

    void DirectX12IndexBuffer::Unbind() const {
    }

} // namespace VECTOR
