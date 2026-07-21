#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>

namespace VECTOR {

    class VulkanContext;
    class VulkanPipeline;

    class VulkanShadowPass {
    public:
        VulkanShadowPass(VulkanContext* context);
        ~VulkanShadowPass();

        // Prevent copying
        VulkanShadowPass(const VulkanShadowPass&) = delete;
        VulkanShadowPass& operator=(const VulkanShadowPass&) = delete;

        void Initialize(VkPipelineLayout pipelineLayout);
        void Shutdown();

        void BeginPass(VkCommandBuffer commandBuffer);
        void EndPass(VkCommandBuffer commandBuffer);

        VkImageView GetImageView() const { return m_ShadowImageView; }
        VkSampler GetSampler() const { return m_ShadowSampler; }
        VkRenderPass GetRenderPass() const { return m_ShadowRenderPass; }
        VulkanPipeline* GetDepthPipeline() const { return m_DepthPipeline.get(); }
        uint32_t GetShadowMapSize() const { return SHADOW_MAP_SIZE; }

    private:
        void CreateResources();
        void CreatePipeline(VkPipelineLayout pipelineLayout);

        VulkanContext* m_Context = nullptr;

        const uint32_t SHADOW_MAP_SIZE = 2048;
        VkRenderPass m_ShadowRenderPass = VK_NULL_HANDLE;
        VkImage m_ShadowImage = VK_NULL_HANDLE;
        VmaAllocation m_ShadowImageAllocation = VK_NULL_HANDLE;
        VkImageView m_ShadowImageView = VK_NULL_HANDLE;
        VkSampler m_ShadowSampler = VK_NULL_HANDLE;
        VkFramebuffer m_ShadowFramebuffer = VK_NULL_HANDLE;

        std::unique_ptr<VulkanPipeline> m_DepthPipeline;
    };

} // namespace VECTOR
