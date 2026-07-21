#include "Engine/Graphics/Vulkan/VulkanDescriptorManager.hpp"
#include "Engine/Graphics/Vulkan/VulkanContext.hpp"
#include "Engine/Core/Logger.hpp"
#include <glm/glm.hpp>
#include <array>

namespace VECTOR {

    VulkanDescriptorManager::VulkanDescriptorManager(VulkanContext* context, uint32_t framesInFlight)
        : m_Context(context), m_FramesInFlight(framesInFlight) {
    }

    VulkanDescriptorManager::~VulkanDescriptorManager() {
        Shutdown();
    }

    void VulkanDescriptorManager::Initialize() {
        CreateLayouts();
        CreatePool();
    }

    void VulkanDescriptorManager::Shutdown() {
        if (!m_Context || !m_Context->GetDevice()) return;
        VkDevice device = m_Context->GetDevice();

        if (m_MainDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, m_MainDescriptorPool, nullptr);
            m_MainDescriptorPool = VK_NULL_HANDLE;
        }
        if (m_PipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }
        if (m_GlobalSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, m_GlobalSetLayout, nullptr);
            m_GlobalSetLayout = VK_NULL_HANDLE;
        }
        if (m_MaterialSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, m_MaterialSetLayout, nullptr);
            m_MaterialSetLayout = VK_NULL_HANDLE;
        }
    }

    void VulkanDescriptorManager::CreateLayouts() {
        VkDevice device = m_Context->GetDevice();

        // 1. Global Set Layout (Set 0: UBOs)
        VkDescriptorSetLayoutBinding perFrameBinding{};
        perFrameBinding.binding = 0;
        perFrameBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        perFrameBinding.descriptorCount = 1;
        perFrameBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding lightBinding{};
        lightBinding.binding = 1;
        lightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lightBinding.descriptorCount = 1;
        lightBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding shadowBinding{};
        shadowBinding.binding = 2;
        shadowBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowBinding.descriptorCount = 1;
        shadowBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { perFrameBinding, lightBinding, shadowBinding };

        VkDescriptorSetLayoutCreateInfo globalLayoutInfo{};
        globalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        globalLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        globalLayoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &globalLayoutInfo, nullptr, &m_GlobalSetLayout) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create global descriptor set layout!");
        }

        // 2. Material Set Layout (Set 1: Diffuse Map)
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
        materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        materialLayoutInfo.bindingCount = 1;
        materialLayoutInfo.pBindings = &samplerLayoutBinding;

        if (vkCreateDescriptorSetLayout(device, &materialLayoutInfo, nullptr, &m_MaterialSetLayout) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create material descriptor set layout!");
        }

        // 3. Pipeline Layout
        VkPushConstantRange pushConstant{};
        pushConstant.offset = 0;
        pushConstant.size = sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(uint32_t) * 2; // model + color + hasTexture + isUnlit
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayout setLayouts[] = { m_GlobalSetLayout, m_MaterialSetLayout };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 2;
        pipelineLayoutInfo.pSetLayouts = setLayouts;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create pipeline layout!");
        }
    }

    void VulkanDescriptorManager::CreatePool() {
        VkDevice device = m_Context->GetDevice();

        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(m_FramesInFlight * 2); // 2 UBOs per frame
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(m_FramesInFlight + 1024); // max textures + shadow maps

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(m_FramesInFlight + 1024);
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_MainDescriptorPool) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create main descriptor pool!");
        }
    }

    std::vector<VkDescriptorSet> VulkanDescriptorManager::AllocateGlobalSets() {
        std::vector<VkDescriptorSetLayout> layouts(m_FramesInFlight, m_GlobalSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_MainDescriptorPool;
        allocInfo.descriptorSetCount = m_FramesInFlight;
        allocInfo.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> globalSets(m_FramesInFlight);
        if (vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, globalSets.data()) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to allocate global descriptor sets!");
        }
        return globalSets;
    }

    VkDescriptorSet VulkanDescriptorManager::AllocateMaterialSet() {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_MainDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_MaterialSetLayout;

        VkDescriptorSet set;
        if (vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, &set) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to allocate material descriptor set!");
            return VK_NULL_HANDLE;
        }
        return set;
    }

} // namespace VECTOR
