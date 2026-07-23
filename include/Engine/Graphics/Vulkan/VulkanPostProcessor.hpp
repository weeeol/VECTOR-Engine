#pragma once

#include "Engine/Graphics/Vulkan/VulkanContext.hpp"
#include "Engine/Graphics/Vulkan/VulkanPipeline.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <memory>

namespace VECTOR {

    struct BloomMip {
        uint32_t width;
        uint32_t height;
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    class VulkanPostProcessor {
    public:
        VulkanPostProcessor(uint32_t width, uint32_t height, VkRenderPass swapchainRenderPass);
        ~VulkanPostProcessor();

        void Process(VkCommandBuffer commandBuffer);
        void ProcessSSAO(VkCommandBuffer commandBuffer, VkImageView normalView, VkImageView depthView, const glm::mat4& projection, const glm::mat4& view);
        void Recreate(uint32_t width, uint32_t height, VkRenderPass swapchainRenderPass);
        
        // Call this during rendering
        void RenderBloom(VkCommandBuffer commandBuffer);
        void RenderFinal(VkCommandBuffer commandBuffer, VkDescriptorSet globalSet, uint32_t currentFrame);

        VkRenderPass GetOffscreenRenderPass() const { return m_OffscreenRenderPass; }
        VkFramebuffer GetOffscreenFramebuffer() const { return m_OffscreenFramebuffer; }
        VkImageView GetOffscreenColorView() const { return m_OffscreenColorView; }
        
        float exposure = 1.0f;
        float bloomStrength = 0.04f;
        float bloomFilterRadius = 0.005f;
        float bloomThreshold = 1.0f;

    private:
        void CreateResources();
        void DestroyResources();
        
        void CreateOffscreenRenderPass();
        void CreatePipelines();
        void CreateDescriptorSets();

        uint32_t m_Width;
        uint32_t m_Height;
        
        VkRenderPass m_SwapchainRenderPass = VK_NULL_HANDLE;
        
        // Offscreen G-Buffer
        VkRenderPass m_OffscreenRenderPass = VK_NULL_HANDLE;
        VkFramebuffer m_OffscreenFramebuffer = VK_NULL_HANDLE;
        
        VkImage m_OffscreenColorImage = VK_NULL_HANDLE;
        VmaAllocation m_OffscreenColorAlloc = VK_NULL_HANDLE;
        VkImageView m_OffscreenColorView = VK_NULL_HANDLE;
        
        VkImage m_OffscreenDepthImage = VK_NULL_HANDLE;
        VmaAllocation m_OffscreenDepthAlloc = VK_NULL_HANDLE;
        VkImageView m_OffscreenDepthView = VK_NULL_HANDLE;
        
        // Bloom
        std::vector<BloomMip> m_BloomMips;
        uint32_t m_MipLevels = 6;
        
        // Render passes for bloom
        VkRenderPass m_BloomRenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_BloomFramebuffers;
        
        // Pipelines
        std::unique_ptr<VulkanPipeline> m_DownsamplePipeline;
        std::unique_ptr<VulkanPipeline> m_UpsamplePipeline;
        std::unique_ptr<VulkanPipeline> m_PostProcessPipeline;
        
        VkPipelineLayout m_DownsamplePipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_UpsamplePipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_PostProcessPipelineLayout = VK_NULL_HANDLE;
        
        // Descriptors
        VkDescriptorSetLayout m_PostProcessSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_BloomSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        
        // One set for post process, one set per mip for downsample/upsample
        VkDescriptorSet m_PostProcessDescriptorSet = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_BloomDescriptorSets;
        
        VkSampler m_ColorSampler = VK_NULL_HANDLE;
    };

} // namespace VECTOR
