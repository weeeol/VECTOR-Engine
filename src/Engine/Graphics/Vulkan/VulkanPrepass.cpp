#include "Engine/Graphics/Vulkan/VulkanPrepass.hpp"
#include "Engine/Graphics/Vulkan/VulkanContext.hpp"
#include "Engine/Graphics/Vulkan/VulkanPipeline.hpp"
#include "Engine/Core/Logger.hpp"
#include <array>

namespace VECTOR {

    VulkanPrepass::VulkanPrepass(VulkanContext* context, uint32_t width, uint32_t height)
        : m_Context(context), m_Width(width), m_Height(height) {
    }

    VulkanPrepass::~VulkanPrepass() {
        Shutdown();
    }

    void VulkanPrepass::Initialize(VkPipelineLayout pipelineLayout) {
        m_PipelineLayout = pipelineLayout;
        CreateResources(m_Width, m_Height);
        CreatePipeline(pipelineLayout);
    }

    void VulkanPrepass::Shutdown() {
        if (!m_Context || !m_Context->GetDevice()) return;
        
        m_Pipeline.reset();
        DestroyResources();
        
        if (m_RenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_Context->GetDevice(), m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }
    }
    
    void VulkanPrepass::DestroyResources() {
        VkDevice device = m_Context->GetDevice();
        VmaAllocator allocator = m_Context->GetAllocator();

        if (m_Sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, m_Sampler, nullptr);
            m_Sampler = VK_NULL_HANDLE;
        }
        
        if (m_Framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
            m_Framebuffer = VK_NULL_HANDLE;
        }
        
        if (m_NormalImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, m_NormalImageView, nullptr);
            m_NormalImageView = VK_NULL_HANDLE;
        }
        if (m_NormalImage != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator, m_NormalImage, m_NormalAllocation);
            m_NormalImage = VK_NULL_HANDLE;
        }
        
        if (m_PositionImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, m_PositionImageView, nullptr);
            m_PositionImageView = VK_NULL_HANDLE;
        }
        if (m_PositionImage != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator, m_PositionImage, m_PositionAllocation);
            m_PositionImage = VK_NULL_HANDLE;
        }
        
        if (m_DepthImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, m_DepthImageView, nullptr);
            m_DepthImageView = VK_NULL_HANDLE;
        }
        if (m_DepthImage != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator, m_DepthImage, m_DepthAllocation);
            m_DepthImage = VK_NULL_HANDLE;
        }
    }

    void VulkanPrepass::Resize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) return;
        if (m_Width == width && m_Height == height) return;
        
        m_Width = width;
        m_Height = height;
        
        vkDeviceWaitIdle(m_Context->GetDevice());
        DestroyResources();
        CreateResources(width, height);
    }

    void VulkanPrepass::CreateResources(uint32_t width, uint32_t height) {
        VkDevice device = m_Context->GetDevice();
        VmaAllocator allocator = m_Context->GetAllocator();

        // If RenderPass isn't created yet, create it.
        if (m_RenderPass == VK_NULL_HANDLE) {
            std::array<VkAttachmentDescription, 3> attachments{};
            
            // 0: Normal (RGBA16F)
            attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
            attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            
            // 1: Position (RGBA16F)
            attachments[1].format = VK_FORMAT_R16G16B16A16_SFLOAT;
            attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            
            // 2: Depth
            attachments[2].format = VK_FORMAT_D32_SFLOAT; // We could get this from swapchain depth format
            attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

            std::array<VkAttachmentReference, 2> colorRefs{};
            colorRefs[0].attachment = 0;
            colorRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorRefs[1].attachment = 1;
            colorRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthRef{};
            depthRef.attachment = 2;
            depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
            subpass.pColorAttachments = colorRefs.data();
            subpass.pDepthStencilAttachment = &depthRef;

            std::array<VkSubpassDependency, 2> dependencies;
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
            renderPassInfo.pDependencies = dependencies.data();

            if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
                VECTOR_LOG_ERROR("Failed to create prepass render pass!");
                return;
            }
        }

        // --- Create Images ---
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        // Normal Image
        vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_NormalImage, &m_NormalAllocation, nullptr);
        // Position Image
        vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_PositionImage, &m_PositionAllocation, nullptr);
        
        // Depth Image
        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_DepthImage, &m_DepthAllocation, nullptr);

        // --- Create Image Views ---
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;

        viewInfo.image = m_NormalImage;
        viewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCreateImageView(device, &viewInfo, nullptr, &m_NormalImageView);

        viewInfo.image = m_PositionImage;
        vkCreateImageView(device, &viewInfo, nullptr, &m_PositionImageView);

        viewInfo.image = m_DepthImage;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        vkCreateImageView(device, &viewInfo, nullptr, &m_DepthImageView);

        // --- Create Sampler ---
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST; // No filtering for GBuffer
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler);

        // --- Create Framebuffer ---
        std::array<VkImageView, 3> fbAttachments = {
            m_NormalImageView,
            m_PositionImageView,
            m_DepthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
        framebufferInfo.pAttachments = fbAttachments.data();
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_Framebuffer) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create prepass framebuffer!");
        }
    }

    void VulkanPrepass::CreatePipeline(VkPipelineLayout pipelineLayout) {
        PipelineConfigInfo config{};
        VulkanPipeline::DefaultPipelineConfigInfo(config);
        config.renderPass = m_RenderPass;
        config.pipelineLayout = pipelineLayout;
        
        // We need 2 color blend attachments because we have 2 color outputs (Normal, Position)
        VkPipelineColorBlendAttachmentState defaultAttachment{};
        defaultAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        defaultAttachment.blendEnable = VK_FALSE;
        
        config.colorBlendAttachments = { defaultAttachment, defaultAttachment };
        
        m_Pipeline = std::make_unique<VulkanPipeline>("assets/engine/shaders/vulkan/prepass.vert.spv", "assets/engine/shaders/vulkan/prepass.frag.spv", config);
    }

    void VulkanPrepass::BeginPass(VkCommandBuffer commandBuffer) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_Framebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {m_Width, m_Height};

        std::array<VkClearValue, 3> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[2].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_Pipeline->Bind(commandBuffer);
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)m_Width;
        viewport.height = (float)m_Height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {m_Width, m_Height};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void VulkanPrepass::EndPass(VkCommandBuffer commandBuffer) {
        vkCmdEndRenderPass(commandBuffer);
    }

} // namespace VECTOR
