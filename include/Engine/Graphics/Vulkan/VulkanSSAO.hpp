#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace VECTOR {

    class VulkanContext;
    class VulkanPipeline;
    class VulkanUniformBuffer;

    class VulkanSSAO {
    public:
        VulkanSSAO(VulkanContext* context, uint32_t width, uint32_t height);
        ~VulkanSSAO();

        void Initialize();
        void Shutdown();
        void Resize(uint32_t width, uint32_t height);

        void Generate(VkCommandBuffer commandBuffer, VkImageView normalView, VkImageView depthView, const glm::mat4& projection, const glm::mat4& view);

        VkImageView GetSSAOImageView() const { return m_BlurImageView; }
        VkSampler GetSampler() const { return m_Sampler; }

        void UpdateDescriptorSets(VkImageView normalView, VkImageView depthView);

    private:
        void CreateResources(uint32_t width, uint32_t height);
        void CreateNoiseTexture();
        void CreatePipelines();
        void CreateDescriptorSets();
        void DestroyResources();

        VulkanContext* m_Context = nullptr;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;

        // Raw SSAO Output
        VkImage m_SSAOImage = VK_NULL_HANDLE;
        VmaAllocation m_SSAOAllocation = VK_NULL_HANDLE;
        VkImageView m_SSAOImageView = VK_NULL_HANDLE;
        VkFramebuffer m_SSAOFramebuffer = VK_NULL_HANDLE;

        // Blurred SSAO Output
        VkImage m_BlurImage = VK_NULL_HANDLE;
        VmaAllocation m_BlurAllocation = VK_NULL_HANDLE;
        VkImageView m_BlurImageView = VK_NULL_HANDLE;
        VkFramebuffer m_BlurFramebuffer = VK_NULL_HANDLE;

        // Noise Texture
        VkImage m_NoiseImage = VK_NULL_HANDLE;
        VmaAllocation m_NoiseAllocation = VK_NULL_HANDLE;
        VkImageView m_NoiseImageView = VK_NULL_HANDLE;
        VkSampler m_NoiseSampler = VK_NULL_HANDLE;

        VkSampler m_Sampler = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_SSAOSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_BlurSetLayout = VK_NULL_HANDLE;
        
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_SSAODescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSet m_BlurDescriptorSet = VK_NULL_HANDLE;

        VkPipelineLayout m_SSAOPipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_BlurPipelineLayout = VK_NULL_HANDLE;

        std::unique_ptr<VulkanPipeline> m_SSAOPipeline;
        std::unique_ptr<VulkanPipeline> m_BlurPipeline;

        std::unique_ptr<VulkanUniformBuffer> m_UBO;

        struct SSAOData {
            glm::vec4 samples[64];
            float radius = 0.5f;
            float bias = 0.025f;
            glm::vec2 screenSize;
        } m_SSAOData;

        std::vector<glm::vec3> m_Kernel;
    };

} // namespace VECTOR
