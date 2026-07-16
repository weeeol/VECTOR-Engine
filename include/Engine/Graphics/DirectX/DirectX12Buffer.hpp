#pragma once

#include "Engine/Graphics/Buffer.hpp"
#include <wrl.h>
#include <d3d12.h>

namespace VECTOR {

    class DirectX12VertexBuffer : public VertexBuffer {
    public:
        DirectX12VertexBuffer(const void* vertices, uint32_t size);
        virtual ~DirectX12VertexBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void SetLayoutType(BufferLayoutType type) override { m_LayoutType = type; }
        virtual BufferLayoutType GetLayoutType() const override { return m_LayoutType; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_BufferView;
        BufferLayoutType m_LayoutType = BufferLayoutType::Mesh3D;
    };

    class DirectX12IndexBuffer : public IndexBuffer {
    public:
        DirectX12IndexBuffer(const uint32_t* indices, uint32_t count);
        virtual ~DirectX12IndexBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual uint32_t GetCount() const override { return m_Count; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_BufferView;
        uint32_t m_Count;
    };

} // namespace VECTOR
