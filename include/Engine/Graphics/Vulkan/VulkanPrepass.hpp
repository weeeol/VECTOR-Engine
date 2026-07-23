#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>

namespace VECTOR {

    class VulkanContext;
    class VulkanPipeline;

    class VulkanPrepass {
    public:
        VulkanPrepass(VulkanContext* context, uint32_t width, uint32_t height);
        ~VulkanPrepass();

        VulkanPrepass(const VulkanPrepass&) = delete;
        VulkanPrepass& operator=(const VulkanPrepass&) = delete;

        void Initialize(VkPipelineLayout pipelineLayout);
        void Shutdown();
        void Resize(uint32_t width, uint32_t height);

        void BeginPass(VkCommandBuffer commandBuffer);
        void EndPass(VkCommandBuffer commandBuffer);

        VkImageView GetNormalImageView() const { return m_NormalImageView; }
        VkImageView GetPositionImageView() const { return m_PositionImageView; }
        VkImageView GetDepthImageView() const { return m_DepthImageView; }
        VkSampler GetSampler() const { return m_Sampler; }
        VkRenderPass GetRenderPass() const { return m_RenderPass; }
        VulkanPipeline* GetPipeline() const { return m_Pipeline.get(); }

    private:
        void CreateResources(uint32_t width, uint32_t height);
        void CreatePipeline(VkPipelineLayout pipelineLayout);
        void DestroyResources();

        VulkanContext* m_Context = nullptr;

        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;

        // Normal Attachment
        VkImage m_NormalImage = VK_NULL_HANDLE;
        VmaAllocation m_NormalAllocation = VK_NULL_HANDLE;
        VkImageView m_NormalImageView = VK_NULL_HANDLE;

        // Position Attachment
        VkImage m_PositionImage = VK_NULL_HANDLE;
        VmaAllocation m_PositionAllocation = VK_NULL_HANDLE;
        VkImageView m_PositionImageView = VK_NULL_HANDLE;

        // Depth Attachment
        VkImage m_DepthImage = VK_NULL_HANDLE;
        VmaAllocation m_DepthAllocation = VK_NULL_HANDLE;
        VkImageView m_DepthImageView = VK_NULL_HANDLE;

        VkSampler m_Sampler = VK_NULL_HANDLE;
        VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

        std::unique_ptr<VulkanPipeline> m_Pipeline;
        
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    };

} // namespace VECTOR
