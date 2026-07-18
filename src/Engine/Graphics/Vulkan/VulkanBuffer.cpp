#include "Engine/Graphics/Vulkan/VulkanBuffer.hpp"
#include "Engine/Graphics/Vulkan/VulkanContext.hpp"
#include "Engine/Core/Logger.hpp"
#include <cstring>

namespace VECTOR {

    VulkanBuffer::VulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) 
        : m_Size(size) 
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;
        
        // VMA_MEMORY_USAGE_CPU_ONLY and VMA_MEMORY_USAGE_CPU_TO_GPU usually need host access
        if (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY || memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU) {
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        if (vmaCreateBuffer(VulkanContext::Get()->GetAllocator(), &bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, nullptr) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create Vulkan buffer via VMA!");
        }
    }

    VulkanBuffer::~VulkanBuffer() {
        if (m_Buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(VulkanContext::Get()->GetAllocator(), m_Buffer, m_Allocation);
        }
    }

    void VulkanBuffer::Map(void** data) {
        vmaMapMemory(VulkanContext::Get()->GetAllocator(), m_Allocation, data);
    }

    void VulkanBuffer::Unmap() {
        vmaUnmapMemory(VulkanContext::Get()->GetAllocator(), m_Allocation);
    }

    void VulkanBuffer::UploadData(const void* data, size_t size) {
        void* mappedData;
        Map(&mappedData);
        std::memcpy(mappedData, data, size);
        Unmap();
    }

} // namespace VECTOR
