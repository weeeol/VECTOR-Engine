#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace VECTOR {

    class VulkanContext;

    class VulkanDescriptorManager {
    public:
        VulkanDescriptorManager(VulkanContext* context, uint32_t framesInFlight);
        ~VulkanDescriptorManager();

        // Prevent copying
        VulkanDescriptorManager(const VulkanDescriptorManager&) = delete;
        VulkanDescriptorManager& operator=(const VulkanDescriptorManager&) = delete;

        void Initialize();
        void Shutdown();

        VkDescriptorSetLayout GetGlobalSetLayout() const { return m_GlobalSetLayout; }
        VkDescriptorSetLayout GetMaterialSetLayout() const { return m_MaterialSetLayout; }
        VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
        VkDescriptorPool GetDescriptorPool() const { return m_MainDescriptorPool; }

        std::vector<VkDescriptorSet> AllocateGlobalSets();
        VkDescriptorSet AllocateMaterialSet();

    private:
        void CreateLayouts();
        void CreatePool();

        VulkanContext* m_Context;
        uint32_t m_FramesInFlight;

        VkDescriptorSetLayout m_GlobalSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_MaterialSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_MainDescriptorPool = VK_NULL_HANDLE;
    };

} // namespace VECTOR
