#define NOMINMAX
#include "Engine/Graphics/DirectX12/DirectX12Mesh.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Core/Logger.hpp"
#include <algorithm>

namespace VECTOR {

    DirectX12Mesh::DirectX12Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
        m_VertexCount = (int)vertices.size();
        m_IndexCount = (int)indices.size();

        size_t vertexBufferSize = sizeof(Vertex) * m_VertexCount;
        size_t indexBufferSize = sizeof(unsigned int) * m_IndexCount;

        // In a real engine, we'd use staging buffers. For simplicity, we use UPLOAD heap here.
        m_VertexBuffer = std::make_unique<DirectX12Buffer>(
            vertexBufferSize,
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_RESOURCE_STATE_GENERIC_READ
        );
        m_VertexBuffer->UploadData(vertices.data(), vertexBufferSize);

        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetResource()->GetGPUVirtualAddress();
        m_VertexBufferView.StrideInBytes = sizeof(Vertex);
        m_VertexBufferView.SizeInBytes = (UINT)vertexBufferSize;

        m_IndexBuffer = std::make_unique<DirectX12Buffer>(
            indexBufferSize,
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_RESOURCE_STATE_GENERIC_READ
        );
        m_IndexBuffer->UploadData(indices.data(), indexBufferSize);

        m_IndexBufferView.BufferLocation = m_IndexBuffer->GetResource()->GetGPUVirtualAddress();
        m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
        m_IndexBufferView.SizeInBytes = (UINT)indexBufferSize;

        ComputeAABB(vertices);
    }

    DirectX12Mesh::~DirectX12Mesh() {
    }

    void DirectX12Mesh::Draw() const {
    }

    void DirectX12Mesh::Bind(ID3D12GraphicsCommandList* commandList) const {
        commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        commandList->IASetIndexBuffer(&m_IndexBufferView);
    }

    void DirectX12Mesh::ComputeAABB(const std::vector<Vertex>& vertices) {
        if (vertices.empty()) {
            m_AABB.center = glm::vec3(0.0f);
            m_AABB.extents = glm::vec3(0.0f);
            return;
        }

        glm::vec3 minExtents = vertices[0].Position;
        glm::vec3 maxExtents = vertices[0].Position;

        for (const auto& vertex : vertices) {
            minExtents.x = (std::min)(minExtents.x, vertex.Position.x);
            minExtents.y = (std::min)(minExtents.y, vertex.Position.y);
            minExtents.z = (std::min)(minExtents.z, vertex.Position.z);

            maxExtents.x = (std::max)(maxExtents.x, vertex.Position.x);
            maxExtents.y = (std::max)(maxExtents.y, vertex.Position.y);
            maxExtents.z = (std::max)(maxExtents.z, vertex.Position.z);
        }

        m_AABB.center = (minExtents + maxExtents) * 0.5f;
        m_AABB.extents = (maxExtents - minExtents) * 0.5f;
    }

} // namespace VECTOR
