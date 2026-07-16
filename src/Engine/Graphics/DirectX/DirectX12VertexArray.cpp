#include "Engine/Graphics/DirectX/DirectX12VertexArray.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/DirectX/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX/DirectX12Buffer.hpp"

namespace VECTOR {

    DirectX12VertexArray::DirectX12VertexArray() {
        VECTOR_LOG_INFO("Creating DirectX12VertexArray (Stub)");
    }

    DirectX12VertexArray::~DirectX12VertexArray() {
    }

    void DirectX12VertexArray::Bind() const {
        auto context = DirectX12Context::Get();
        if (!context) return;
        
        auto cmdList = context->GetCommandList();
        
        if (!m_VertexBuffers.empty()) {
            std::vector<D3D12_VERTEX_BUFFER_VIEW> views;
            for (const auto& vb : m_VertexBuffers) {
                auto dxVb = dynamic_cast<DirectX12VertexBuffer*>(vb.get());
                if (dxVb) {
                    views.push_back(dxVb->GetView());
                }
            }
            if (!views.empty()) {
                cmdList->IASetVertexBuffers(0, (UINT)views.size(), views.data());
            }
        }

        if (m_IndexBuffer) {
            auto dxIb = dynamic_cast<DirectX12IndexBuffer*>(m_IndexBuffer.get());
            if (dxIb) {
                auto view = dxIb->GetView();
                cmdList->IASetIndexBuffer(&view);
            }
        }
    }

    void DirectX12VertexArray::Unbind() const {
        // Unbinding is not strictly necessary in DX12 as the next bind will overwrite it
    }

    void DirectX12VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer) {
        m_VertexBuffers.push_back(vertexBuffer);
    }

    void DirectX12VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) {
        m_IndexBuffer = indexBuffer;
    }

} // namespace VECTOR
