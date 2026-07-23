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
        if (m_ObjectSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, m_ObjectSetLayout, nullptr);
            m_ObjectSetLayout = VK_NULL_HANDLE;
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

        // 2. Material Set Layout (Set 1: Diffuse, Normal, MetallicRoughness, AO)
        std::vector<VkDescriptorSetLayoutBinding> materialBindings(4);
        
        // Albedo map
        materialBindings[0].binding = 0;
        materialBindings[0].descriptorCount = 1;
        materialBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        materialBindings[0].pImmutableSamplers = nullptr;
        materialBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Normal map
        materialBindings[1].binding = 1;
        materialBindings[1].descriptorCount = 1;
        materialBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        materialBindings[1].pImmutableSamplers = nullptr;
        materialBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Metallic/Roughness map
        materialBindings[2].binding = 2;
        materialBindings[2].descriptorCount = 1;
        materialBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        materialBindings[2].pImmutableSamplers = nullptr;
        materialBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // AO map
        materialBindings[3].binding = 3;
        materialBindings[3].descriptorCount = 1;
        materialBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        materialBindings[3].pImmutableSamplers = nullptr;
        materialBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
        materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        materialLayoutInfo.bindingCount = static_cast<uint32_t>(materialBindings.size());
        materialLayoutInfo.pBindings = materialBindings.data();

        if (vkCreateDescriptorSetLayout(device, &materialLayoutInfo, nullptr, &m_MaterialSetLayout) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create material descriptor set layout!");
        }

        // 3. Object Set Layout (Set 2: Bones UBO)
        VkDescriptorSetLayoutBinding objectLayoutBinding{};
        objectLayoutBinding.binding = 0;
        objectLayoutBinding.descriptorCount = 1;
        objectLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        objectLayoutBinding.pImmutableSamplers = nullptr;
        objectLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo objectLayoutInfo{};
        objectLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        objectLayoutInfo.bindingCount = 1;
        objectLayoutInfo.pBindings = &objectLayoutBinding;

        if (vkCreateDescriptorSetLayout(device, &objectLayoutInfo, nullptr, &m_ObjectSetLayout) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create object descriptor set layout!");
        }

        // 4. Pipeline Layout
        VkPushConstantRange pushConstant{};
        pushConstant.offset = 0;
        pushConstant.size = 128; // model + color + hasTexture + isUnlit + PBR params
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayout setLayouts[] = { m_GlobalSetLayout, m_MaterialSetLayout, m_ObjectSetLayout };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 3;
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
        poolSizes[0].descriptorCount = static_cast<uint32_t>(m_FramesInFlight * 2 + 4096); // 2 UBOs per frame + object UBOs
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(m_FramesInFlight + 1024); // max textures + shadow maps

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(m_FramesInFlight + 1024 + 4096);
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

    VkDescriptorSet VulkanDescriptorManager::AllocateObjectSet() {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_MainDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_ObjectSetLayout;

        VkDescriptorSet set;
        if (vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, &set) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to allocate object descriptor set!");
            return VK_NULL_HANDLE;
        }
        return set;
    }

} // namespace VECTOR
