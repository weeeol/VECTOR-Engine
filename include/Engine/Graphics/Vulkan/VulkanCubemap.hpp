#pragma once
#include <string>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>

namespace VECTOR {

    class VulkanCubemap {
    public:
        // Loads 6 separate HDR faces for the cubemap.
        // Order: Right, Left, Top, Bottom, Front, Back
        VulkanCubemap(const std::vector<std::string>& facePaths);
        ~VulkanCubemap();

        VkImageView GetImageView() const { return m_ImageView; }
        VkSampler GetSampler() const { return m_Sampler; }

    private:
        void LoadFaces(const std::vector<std::string>& facePaths);
        void CreateCubemapImage(uint32_t width, uint32_t height, void* data);

        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        VkImage m_Image = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;
    };

} // namespace VECTOR
