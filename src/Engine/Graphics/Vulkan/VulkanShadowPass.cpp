#include "Engine/Graphics/Vulkan/VulkanShadowPass.hpp"
#include "Engine/Graphics/Vulkan/VulkanContext.hpp"
#include "Engine/Graphics/Vulkan/VulkanPipeline.hpp"
#include "Engine/Core/Logger.hpp"
#include <array>

namespace VECTOR {

    VulkanShadowPass::VulkanShadowPass(VulkanContext* context) : m_Context(context) {
    }

    VulkanShadowPass::~VulkanShadowPass() {
        Shutdown();
    }

    void VulkanShadowPass::Initialize(VkPipelineLayout pipelineLayout) {
        CreateResources();
        CreatePipeline(pipelineLayout);
    }

    void VulkanShadowPass::Shutdown() {
        if (!m_Context || !m_Context->GetDevice()) return;
        VkDevice device = m_Context->GetDevice();

        m_DepthPipeline.reset();

        if (m_ShadowSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, m_ShadowSampler, nullptr);
            m_ShadowSampler = VK_NULL_HANDLE;
        }
        if (m_ShadowImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, m_ShadowImageView, nullptr);
            m_ShadowImageView = VK_NULL_HANDLE;
        }
        if (m_ShadowImage != VK_NULL_HANDLE) {
            vmaDestroyImage(m_Context->GetAllocator(), m_ShadowImage, m_ShadowImageAllocation);
            m_ShadowImage = VK_NULL_HANDLE;
            m_ShadowImageAllocation = VK_NULL_HANDLE;
        }
        if (m_ShadowFramebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, m_ShadowFramebuffer, nullptr);
            m_ShadowFramebuffer = VK_NULL_HANDLE;
        }
        if (m_ShadowRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, m_ShadowRenderPass, nullptr);
            m_ShadowRenderPass = VK_NULL_HANDLE;
        }
    }

    void VulkanShadowPass::CreateResources() {
        VkDevice device = m_Context->GetDevice();
        VmaAllocator allocator = m_Context->GetAllocator();

        // 1. Create Render Pass
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_ShadowRenderPass) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create shadow render pass!");
        }

        // 2. Create Image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = SHADOW_MAP_SIZE;
        imageInfo.extent.height = SHADOW_MAP_SIZE;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_ShadowImage, &m_ShadowImageAllocation, nullptr) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create shadow image!");
        }

        // 3. Create ImageView
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_ShadowImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &m_ShadowImageView) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create shadow image view!");
        }

        // 3.5. Transition image layout to SHADER_READ_ONLY_OPTIMAL
        {
            VkCommandPool tempPool;
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            poolInfo.queueFamilyIndex = m_Context->GetGraphicsQueueFamilyIndex();
            vkCreateCommandPool(device, &poolInfo, nullptr, &tempPool);

            VkCommandBufferAllocateInfo cmdAllocInfo{};
            cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdAllocInfo.commandPool = tempPool;
            cmdAllocInfo.commandBufferCount = 1;

            VkCommandBuffer cmd;
            vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(cmd, &beginInfo);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = m_ShadowImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            vkEndCommandBuffer(cmd);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmd;

            VkQueue graphicsQueue = m_Context->GetGraphicsQueue();
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);

            vkFreeCommandBuffers(device, tempPool, 1, &cmd);
            vkDestroyCommandPool(device, tempPool, nullptr);
        }

        // 4. Create Sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &m_ShadowSampler) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create shadow sampler!");
        }

        // 5. Create Framebuffer
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_ShadowRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &m_ShadowImageView;
        framebufferInfo.width = SHADOW_MAP_SIZE;
        framebufferInfo.height = SHADOW_MAP_SIZE;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_ShadowFramebuffer) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create shadow framebuffer!");
        }
    }

    void VulkanShadowPass::CreatePipeline(VkPipelineLayout pipelineLayout) {
        PipelineConfigInfo depthConfig{};
        VulkanPipeline::DefaultPipelineConfigInfo(depthConfig);
        depthConfig.renderPass = m_ShadowRenderPass;
        depthConfig.pipelineLayout = pipelineLayout;
        depthConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        depthConfig.rasterizationInfo.depthBiasEnable = VK_TRUE;
        depthConfig.colorBlendInfo.attachmentCount = 0;
        depthConfig.colorBlendInfo.pAttachments = nullptr;
        
        m_DepthPipeline = std::make_unique<VulkanPipeline>("assets/engine/shaders/vulkan/depth.vert.spv", "assets/engine/shaders/vulkan/depth.frag.spv", depthConfig);
    }

    void VulkanShadowPass::BeginPass(VkCommandBuffer commandBuffer) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_ShadowRenderPass;
        renderPassInfo.framebuffer = m_ShadowFramebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE};
        
        VkClearValue clearDepth;
        clearDepth.depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearDepth;
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(SHADOW_MAP_SIZE);
        viewport.height = static_cast<float>(SHADOW_MAP_SIZE);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void VulkanShadowPass::EndPass(VkCommandBuffer commandBuffer) {
        vkCmdEndRenderPass(commandBuffer);
    }

} // namespace VECTOR
