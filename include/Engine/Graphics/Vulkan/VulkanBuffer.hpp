#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace VECTOR {

    class VulkanBuffer {
    public:
        VulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
        ~VulkanBuffer();

        VulkanBuffer(const VulkanBuffer&) = delete;
        VulkanBuffer& operator=(const VulkanBuffer&) = delete;

        VkBuffer GetBuffer() const { return m_Buffer; }
        VmaAllocation GetAllocation() const { return m_Allocation; }

        void Map(void** data);
        void Unmap();

        void UploadData(const void* data, size_t size);

    private:
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        VkDeviceSize m_Size = 0;
    };

} // namespace VECTOR
