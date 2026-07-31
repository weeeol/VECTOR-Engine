#pragma once
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Buffer.hpp"
#include <memory>
#include <vector>

namespace VECTOR {

    class DirectX12Mesh : public Mesh {
    public:
        DirectX12Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
        virtual ~DirectX12Mesh();

        virtual void Draw() const override; // No-op in DX12
        
        void Bind(ID3D12GraphicsCommandList* commandList) const;
        virtual int GetIndexCount() const override { return m_IndexCount; }
        uint32_t GetVertexCount() const { return m_VertexCount; }

        D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_VertexBufferView; }
        D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return m_IndexBufferView; }

        virtual const AABB& GetAABB() const override { return m_AABB; }

    private:
        void ComputeAABB(const std::vector<Vertex>& vertices);
        
        std::unique_ptr<DirectX12Buffer> m_VertexBuffer;
        std::unique_ptr<DirectX12Buffer> m_IndexBuffer;
        
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
        D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

        int m_IndexCount = 0;
        int m_VertexCount = 0;
        AABB m_AABB;
    };

} // namespace VECTOR
