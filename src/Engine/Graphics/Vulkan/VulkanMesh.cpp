#include "Engine/Graphics/Vulkan/VulkanMesh.hpp"
#include "Engine/Graphics/Vulkan/VulkanContext.hpp"
#include "Engine/Core/Logger.hpp"
#include <algorithm>

namespace VECTOR {

    // Helper to upload via staging buffer
    static void UploadBufferData(VulkanBuffer& destBuffer, const void* data, size_t size) {
        VECTOR_LOG_INFO("UploadBufferData: Allocating staging buffer...");
        VulkanBuffer stagingBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        VECTOR_LOG_INFO("UploadBufferData: Uploading data to staging buffer...");
        stagingBuffer.UploadData(data, size);

        VkDevice device = VulkanContext::Get()->GetDevice();
        VECTOR_LOG_INFO("UploadBufferData: Creating command pool...");
        VkCommandPool tempPool;
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = VulkanContext::Get()->GetGraphicsQueueFamilyIndex();
        vkCreateCommandPool(device, &poolInfo, nullptr, &tempPool);

        VECTOR_LOG_INFO("UploadBufferData: Allocating command buffer...");
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = tempPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);

        VECTOR_LOG_INFO("UploadBufferData: Recording command buffer...");
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(cmd, stagingBuffer.GetBuffer(), destBuffer.GetBuffer(), 1, &copyRegion);

        vkEndCommandBuffer(cmd);

        VECTOR_LOG_INFO("UploadBufferData: Submitting to queue...");
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        VkQueue graphicsQueue = VulkanContext::Get()->GetGraphicsQueue();
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        
        VECTOR_LOG_INFO("UploadBufferData: Waiting for queue idle...");
        vkQueueWaitIdle(graphicsQueue); // Wait for transfer to complete

        VECTOR_LOG_INFO("UploadBufferData: Freeing resources...");
        vkFreeCommandBuffers(device, tempPool, 1, &cmd);
        vkDestroyCommandPool(device, tempPool, nullptr);
        VECTOR_LOG_INFO("UploadBufferData: Done!");
    }

    VulkanMesh::VulkanMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) 
        : m_IndexCount(static_cast<int>(indices.size())), m_VertexCount(static_cast<int>(vertices.size())) 
    {
        size_t vertexSize = sizeof(Vertex) * vertices.size();
        size_t indexSize = sizeof(unsigned int) * indices.size();

        m_VertexBuffer = std::make_unique<VulkanBuffer>(
            vertexSize, 
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            VMA_MEMORY_USAGE_GPU_ONLY
        );

        m_IndexBuffer = std::make_unique<VulkanBuffer>(
            indexSize, 
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
            VMA_MEMORY_USAGE_GPU_ONLY
        );

        UploadBufferData(*m_VertexBuffer, vertices.data(), vertexSize);
        UploadBufferData(*m_IndexBuffer, indices.data(), indexSize);

        ComputeAABB(vertices);
    }

    VulkanMesh::~VulkanMesh() {
    }

    void VulkanMesh::Draw() const {
        // No-op for Vulkan
    }

    void VulkanMesh::Bind(VkCommandBuffer commandBuffer) const {
        VkBuffer vertexBuffers[] = { m_VertexBuffer->GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanMesh::ComputeAABB(const std::vector<Vertex>& vertices) {
        if (vertices.empty()) return;
        
        glm::vec3 minPoint = vertices[0].Position;
        glm::vec3 maxPoint = vertices[0].Position;

        for (const auto& v : vertices) {
            minPoint.x = std::min(minPoint.x, v.Position.x);
            minPoint.y = std::min(minPoint.y, v.Position.y);
            minPoint.z = std::min(minPoint.z, v.Position.z);

            maxPoint.x = std::max(maxPoint.x, v.Position.x);
            maxPoint.y = std::max(maxPoint.y, v.Position.y);
            maxPoint.z = std::max(maxPoint.z, v.Position.z);
        }

        m_AABB.center = (maxPoint + minPoint) * 0.5f;
        m_AABB.extents = (maxPoint - minPoint) * 0.5f;
    }

} // namespace VECTOR
