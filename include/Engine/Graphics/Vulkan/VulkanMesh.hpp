#pragma once
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Vulkan/VulkanBuffer.hpp"
#include <memory>
#include <vector>

namespace VECTOR {

    class VulkanMesh : public Mesh {
    public:
        VulkanMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
        virtual ~VulkanMesh();

        virtual void Draw() const override; // No-op in Vulkan, use Bind/Draw explicitly via command buffer
        
        void Bind(VkCommandBuffer commandBuffer) const;
        virtual int GetIndexCount() const override { return m_IndexCount; }
        uint32_t GetVertexCount() const { return m_VertexCount; }

        VulkanBuffer* GetVertexBuffer() const { return m_VertexBuffer.get(); }
        VulkanBuffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }

        virtual const AABB& GetAABB() const override { return m_AABB; }

    private:
        void ComputeAABB(const std::vector<Vertex>& vertices);
        
        std::unique_ptr<VulkanBuffer> m_VertexBuffer;
        std::unique_ptr<VulkanBuffer> m_IndexBuffer;
        int m_IndexCount = 0;
        int m_VertexCount = 0;
        AABB m_AABB;
    };

} // namespace VECTOR
