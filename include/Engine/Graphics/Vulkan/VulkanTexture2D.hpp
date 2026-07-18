#pragma once
#include "Engine/Graphics/Texture2D.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace VECTOR {

    class VulkanTexture2D : public Texture2D {
    public:
        VulkanTexture2D(const std::string& path);
        VulkanTexture2D(uint32_t width, uint32_t height, void* data, uint32_t channels);
        virtual ~VulkanTexture2D();

        virtual void Bind(unsigned int slot = 0) const override;
        virtual void Unbind() const override;

        virtual int GetWidth() const override { return m_Width; }
        virtual int GetHeight() const override { return m_Height; }
        virtual unsigned int GetID() const override { return 0; } // OpenGL ID, not used in Vulkan directly

        VkImageView GetImageView() const { return m_ImageView; }
        VkSampler GetSampler() const { return m_Sampler; }

    private:
        void LoadFromFile(const std::string& path);
        void CreateTextureFromData(void* data, uint32_t width, uint32_t height, uint32_t channels);

        std::string m_Path;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint32_t m_Channels = 0;

        VkImage m_Image = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;
    };

} // namespace VECTOR
