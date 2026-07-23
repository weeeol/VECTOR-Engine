#include "Engine/Graphics/Vulkan/VulkanPostProcessor.hpp"
#include "Engine/Core/Logger.hpp"
#include <array>

namespace VECTOR {

    VulkanPostProcessor::VulkanPostProcessor(uint32_t width, uint32_t height, VkRenderPass swapchainRenderPass)
        : m_Width(width), m_Height(height), m_SwapchainRenderPass(swapchainRenderPass) {
        
        CreateOffscreenRenderPass();
        CreateResources();
        CreateDescriptorSets();
        CreatePipelines();
    }

    VulkanPostProcessor::~VulkanPostProcessor() {
        DestroyResources();
        
        VkDevice device = VulkanContext::Get()->GetDevice();
        vkDestroyRenderPass(device, m_OffscreenRenderPass, nullptr);
        if (m_BloomRenderPass) {
            vkDestroyRenderPass(device, m_BloomRenderPass, nullptr);
        }
        
        // Destructors of VulkanPipeline handle pipeline destruction
    }

    void VulkanPostProcessor::Recreate(uint32_t width, uint32_t height, VkRenderPass swapchainRenderPass) {
        m_Width = width;
        m_Height = height;
        m_SwapchainRenderPass = swapchainRenderPass;
        
        DestroyResources();
        CreateResources();
        CreateDescriptorSets();
        
        // Recreate pipelines because the swapchain render pass has changed
        CreatePipelines();
    }
    
    namespace {
        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImage& image, VmaAllocation& allocation) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            if (vmaCreateImage(VulkanContext::Get()->GetAllocator(), &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
                VECTOR_LOG_ERROR("Failed to create image!");
            }
        }

        VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            if (vkCreateImageView(VulkanContext::Get()->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                VECTOR_LOG_ERROR("Failed to create image view!");
            }
            return imageView;
        }

        void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
            VkDevice device = VulkanContext::Get()->GetDevice();
            
            VkCommandPool tempPool;
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            poolInfo.queueFamilyIndex = VulkanContext::Get()->GetGraphicsQueueFamilyIndex();
            vkCreateCommandPool(device, &poolInfo, nullptr, &tempPool);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = tempPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer cmd;
            vkAllocateCommandBuffers(device, &allocInfo, &cmd);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(cmd, &beginInfo);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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

            VkQueue graphicsQueue = VulkanContext::Get()->GetGraphicsQueue();
            {
                std::lock_guard<std::mutex> lock(VulkanContext::Get()->GetGraphicsQueueMutex());
                vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
                vkQueueWaitIdle(graphicsQueue);
            }

            vkFreeCommandBuffers(device, tempPool, 1, &cmd);
            vkDestroyCommandPool(device, tempPool, nullptr);
        }
    }

    void VulkanPostProcessor::DestroyResources() {
        VkDevice device = VulkanContext::Get()->GetDevice();
        VmaAllocator allocator = VulkanContext::Get()->GetAllocator();

        if (m_BloomMips.size() > 0) {
            for (auto& mip : m_BloomMips) {
                if (mip.view) vkDestroyImageView(device, mip.view, nullptr);
                if (mip.image) vmaDestroyImage(allocator, mip.image, mip.allocation);
            }
            m_BloomMips.clear();
        }
        for (auto fb : m_BloomFramebuffers) {
            vkDestroyFramebuffer(device, fb, nullptr);
        }
        m_BloomFramebuffers.clear();


        if (m_BloomSetLayout) {
            vkDestroyDescriptorSetLayout(device, m_BloomSetLayout, nullptr);
            m_BloomSetLayout = VK_NULL_HANDLE;
        }
        if (m_PostProcessSetLayout) {
            vkDestroyDescriptorSetLayout(device, m_PostProcessSetLayout, nullptr);
            m_PostProcessSetLayout = VK_NULL_HANDLE;
        }
        if (m_DownsamplePipelineLayout) {
            vkDestroyPipelineLayout(device, m_DownsamplePipelineLayout, nullptr);
            m_DownsamplePipelineLayout = VK_NULL_HANDLE;
        }
        if (m_UpsamplePipelineLayout) {
            vkDestroyPipelineLayout(device, m_UpsamplePipelineLayout, nullptr);
            m_UpsamplePipelineLayout = VK_NULL_HANDLE;
        }
        if (m_PostProcessPipelineLayout) {
            vkDestroyPipelineLayout(device, m_PostProcessPipelineLayout, nullptr);
            m_PostProcessPipelineLayout = VK_NULL_HANDLE;
        }
        if (m_DescriptorPool) {
            vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }
        if (m_ColorSampler) {
            vkDestroySampler(device, m_ColorSampler, nullptr);
            m_ColorSampler = VK_NULL_HANDLE;
        }
        if (m_OffscreenFramebuffer) {
            vkDestroyFramebuffer(device, m_OffscreenFramebuffer, nullptr);
            m_OffscreenFramebuffer = VK_NULL_HANDLE;
        }
        if (m_OffscreenColorView) {
            vkDestroyImageView(device, m_OffscreenColorView, nullptr);
            m_OffscreenColorView = VK_NULL_HANDLE;
        }
        if (m_OffscreenColorImage) {
            vmaDestroyImage(allocator, m_OffscreenColorImage, m_OffscreenColorAlloc);
            m_OffscreenColorImage = VK_NULL_HANDLE;
        }
        if (m_OffscreenDepthView) {
            vkDestroyImageView(device, m_OffscreenDepthView, nullptr);
            m_OffscreenDepthView = VK_NULL_HANDLE;
        }
        if (m_OffscreenDepthImage) {
            vmaDestroyImage(allocator, m_OffscreenDepthImage, m_OffscreenDepthAlloc);
            m_OffscreenDepthImage = VK_NULL_HANDLE;
        }
    }
    
    void VulkanPostProcessor::CreateOffscreenRenderPass() {
        VkDevice device = VulkanContext::Get()->GetDevice();

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT; // We could find supported format, but d32 is standard
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_OffscreenRenderPass) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create offscreen render pass!");
        }

        // Bloom Render Pass (Color only, no depth, loadOp LOAD since we blend in upsample)
        VkAttachmentDescription bloomAttachment{};
        bloomAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        bloomAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        bloomAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Important for additive blending in upsample
        bloomAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        bloomAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        bloomAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        bloomAttachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
        bloomAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkAttachmentReference bloomAttachmentRef{};
        bloomAttachmentRef.attachment = 0;
        bloomAttachmentRef.layout = VK_IMAGE_LAYOUT_GENERAL;

        VkSubpassDescription bloomSubpass{};
        bloomSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        bloomSubpass.colorAttachmentCount = 1;
        bloomSubpass.pColorAttachments = &bloomAttachmentRef;

        std::array<VkSubpassDependency, 2> bloomDependencies;
        bloomDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        bloomDependencies[0].dstSubpass = 0;
        bloomDependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        bloomDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        bloomDependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        bloomDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        bloomDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        bloomDependencies[1].srcSubpass = 0;
        bloomDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        bloomDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        bloomDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        bloomDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        bloomDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        bloomDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo bloomRenderPassInfo{};
        bloomRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        bloomRenderPassInfo.attachmentCount = 1;
        bloomRenderPassInfo.pAttachments = &bloomAttachment;
        bloomRenderPassInfo.subpassCount = 1;
        bloomRenderPassInfo.pSubpasses = &bloomSubpass;
        bloomRenderPassInfo.dependencyCount = static_cast<uint32_t>(bloomDependencies.size());
        bloomRenderPassInfo.pDependencies = bloomDependencies.data();

        if (vkCreateRenderPass(device, &bloomRenderPassInfo, nullptr, &m_BloomRenderPass) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create bloom render pass!");
        }
    }
    
    void VulkanPostProcessor::CreateResources() {
        VkDevice device = VulkanContext::Get()->GetDevice();
        
        // 1. Create Offscreen Color Image
        CreateImage(m_Width, m_Height, VK_FORMAT_R16G16B16A16_SFLOAT,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    m_OffscreenColorImage, m_OffscreenColorAlloc);
        m_OffscreenColorView = CreateImageView(m_OffscreenColorImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
        
        // 2. Create Offscreen Depth Image
        CreateImage(m_Width, m_Height, VK_FORMAT_D32_SFLOAT,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    m_OffscreenDepthImage, m_OffscreenDepthAlloc);
        m_OffscreenDepthView = CreateImageView(m_OffscreenDepthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);

        // 3. Create Offscreen Framebuffer
        std::array<VkImageView, 2> attachments = { m_OffscreenColorView, m_OffscreenDepthView };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_OffscreenRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_Width;
        framebufferInfo.height = m_Height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_OffscreenFramebuffer) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create offscreen framebuffer!");
        }

        // 4. Create Sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &m_ColorSampler) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create color sampler!");
        }

        // 5. Create Bloom Mips and Framebuffers
        m_BloomMips.resize(m_MipLevels);
        m_BloomFramebuffers.resize(m_MipLevels);

        uint32_t mipWidth = m_Width / 2;
        uint32_t mipHeight = m_Height / 2;

        for (uint32_t i = 0; i < m_MipLevels; i++) {
            m_BloomMips[i].width = mipWidth;
            m_BloomMips[i].height = mipHeight;

            CreateImage(mipWidth, mipHeight, VK_FORMAT_R16G16B16A16_SFLOAT,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        m_BloomMips[i].image, m_BloomMips[i].allocation);
            m_BloomMips[i].view = CreateImageView(m_BloomMips[i].image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
            TransitionImageLayout(m_BloomMips[i].image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = m_BloomRenderPass;
            fbInfo.attachmentCount = 1;
            fbInfo.pAttachments = &m_BloomMips[i].view;
            fbInfo.width = mipWidth;
            fbInfo.height = mipHeight;
            fbInfo.layers = 1;

            if (vkCreateFramebuffer(device, &fbInfo, nullptr, &m_BloomFramebuffers[i]) != VK_SUCCESS) {
                VECTOR_LOG_ERROR("Failed to create bloom framebuffer!");
            }

            mipWidth = std::max(1u, mipWidth / 2);
            mipHeight = std::max(1u, mipHeight / 2);
        }
    }
    
    void VulkanPostProcessor::CreatePipelines() {
        PipelineConfigInfo configInfo{};
        VulkanPipeline::DefaultPipelineConfigInfo(configInfo);

        // Turn off depth testing for fullscreen quads
        configInfo.depthStencilInfo.depthTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
        configInfo.emptyVertexInput = true;
        
        // No backface culling for full screen tri
        configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        
        // Setup additive blending for upsampling
        // Downsample pipeline DOES NOT need blending (it overwrites)
        configInfo.colorBlendAttachments[0].blendEnable = VK_FALSE;

        // Downsample Pipeline
        configInfo.renderPass = m_BloomRenderPass;
        configInfo.pipelineLayout = VK_NULL_HANDLE; // Will be created internally if we provide layout
        
        // Create downsample layout
        VkPipelineLayoutCreateInfo downsampleLayoutInfo{};
        downsampleLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        downsampleLayoutInfo.setLayoutCount = 1;
        downsampleLayoutInfo.pSetLayouts = &m_BloomSetLayout;
        
        VkPushConstantRange downPushRange{};
        downPushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        downPushRange.offset = 0;
        downPushRange.size = sizeof(float) * 3; // vec2 srcRes, float threshold
        
        downsampleLayoutInfo.pushConstantRangeCount = 1;
        downsampleLayoutInfo.pPushConstantRanges = &downPushRange;
        
        VkDevice device = VulkanContext::Get()->GetDevice();
        VkPipelineLayout downLayout;
        vkCreatePipelineLayout(device, &downsampleLayoutInfo, nullptr, &downLayout);
        configInfo.pipelineLayout = downLayout;
        m_DownsamplePipelineLayout = downLayout;

        m_DownsamplePipeline = std::make_unique<VulkanPipeline>(
            "assets/engine/shaders/vulkan/fullscreen.vert.spv",
            "assets/engine/shaders/vulkan/bloom_downsample.frag.spv",
            configInfo
        );

        // Upsample pipeline NEEDS additive blending
        configInfo.colorBlendAttachments[0].blendEnable = VK_TRUE;
        configInfo.colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        configInfo.colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        configInfo.colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
        configInfo.colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        configInfo.colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        configInfo.colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;

        // Upsample Pipeline (using same additive blending config)
        VkPipelineLayoutCreateInfo upLayoutInfo{};
        upLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        upLayoutInfo.setLayoutCount = 1;
        upLayoutInfo.pSetLayouts = &m_BloomSetLayout;
        
        VkPushConstantRange upPushRange{};
        upPushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        upPushRange.offset = 0;
        upPushRange.size = sizeof(float); // float filterRadius
        
        upLayoutInfo.pushConstantRangeCount = 1;
        upLayoutInfo.pPushConstantRanges = &upPushRange;
        
        VkPipelineLayout upLayout;
        vkCreatePipelineLayout(device, &upLayoutInfo, nullptr, &upLayout);
        configInfo.pipelineLayout = upLayout;
        m_UpsamplePipelineLayout = upLayout;

        m_UpsamplePipeline = std::make_unique<VulkanPipeline>(
            "assets/engine/shaders/vulkan/fullscreen.vert.spv",
            "assets/engine/shaders/vulkan/bloom_upsample.frag.spv",
            configInfo
        );

        // Final Post Process pipeline overwrites Swapchain (no blending needed)
        configInfo.renderPass = m_SwapchainRenderPass;
        configInfo.colorBlendAttachments[0].blendEnable = VK_FALSE;
        
        VkPipelineLayoutCreateInfo postLayoutInfo{};
        postLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        postLayoutInfo.setLayoutCount = 1;
        postLayoutInfo.pSetLayouts = &m_PostProcessSetLayout;
        
        VkPushConstantRange postPushRange{};
        postPushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        postPushRange.offset = 0;
        postPushRange.size = sizeof(float) * 2; // float exposure, float bloomStrength
        
        postLayoutInfo.pushConstantRangeCount = 1;
        postLayoutInfo.pPushConstantRanges = &postPushRange;
        
        VkPipelineLayout postLayout;
        vkCreatePipelineLayout(device, &postLayoutInfo, nullptr, &postLayout);
        configInfo.pipelineLayout = postLayout;
        m_PostProcessPipelineLayout = postLayout;

        m_PostProcessPipeline = std::make_unique<VulkanPipeline>(
            "assets/engine/shaders/vulkan/fullscreen.vert.spv",
            "assets/engine/shaders/vulkan/postprocess.frag.spv",
            configInfo
        );
    }
    
    void VulkanPostProcessor::CreateDescriptorSets() {
        VkDevice device = VulkanContext::Get()->GetDevice();

        // 1. Post Process Layout (2 combined image samplers)
        std::array<VkDescriptorSetLayoutBinding, 2> postBindings{};
        postBindings[0].binding = 0;
        postBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        postBindings[0].descriptorCount = 1;
        postBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        postBindings[1].binding = 1;
        postBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        postBindings[1].descriptorCount = 1;
        postBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo postLayoutInfo{};
        postLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        postLayoutInfo.bindingCount = static_cast<uint32_t>(postBindings.size());
        postLayoutInfo.pBindings = postBindings.data();

        if (vkCreateDescriptorSetLayout(device, &postLayoutInfo, nullptr, &m_PostProcessSetLayout) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create post process descriptor set layout!");
        }

        // 2. Bloom Layout (1 combined image sampler)
        VkDescriptorSetLayoutBinding bloomBinding{};
        bloomBinding.binding = 0;
        bloomBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bloomBinding.descriptorCount = 1;
        bloomBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo bloomLayoutInfo{};
        bloomLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        bloomLayoutInfo.bindingCount = 1;
        bloomLayoutInfo.pBindings = &bloomBinding;

        if (vkCreateDescriptorSetLayout(device, &bloomLayoutInfo, nullptr, &m_BloomSetLayout) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create bloom descriptor set layout!");
        }

        // 3. Pool
        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 2 + m_MipLevels + 1; // post process + mips + main offscreen

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1 + m_MipLevels + 1;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create post process descriptor pool!");
        }

        // 4. Allocate and Update Post Process Set
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_PostProcessSetLayout;

        if (vkAllocateDescriptorSets(device, &allocInfo, &m_PostProcessDescriptorSet) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to allocate post process descriptor set!");
        }

        VkDescriptorImageInfo screenInfo{};
        screenInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        screenInfo.imageView = m_OffscreenColorView;
        screenInfo.sampler = m_ColorSampler;

        VkDescriptorImageInfo bloomInfo{};
        bloomInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        bloomInfo.imageView = m_BloomMips[0].view; // Highest res mip
        bloomInfo.sampler = m_ColorSampler;

        std::array<VkWriteDescriptorSet, 2> postWrites{};
        postWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        postWrites[0].dstSet = m_PostProcessDescriptorSet;
        postWrites[0].dstBinding = 0;
        postWrites[0].dstArrayElement = 0;
        postWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        postWrites[0].descriptorCount = 1;
        postWrites[0].pImageInfo = &screenInfo;

        postWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        postWrites[1].dstSet = m_PostProcessDescriptorSet;
        postWrites[1].dstBinding = 1;
        postWrites[1].dstArrayElement = 0;
        postWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        postWrites[1].descriptorCount = 1;
        postWrites[1].pImageInfo = &bloomInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(postWrites.size()), postWrites.data(), 0, nullptr);

        // 5. Allocate and Update Bloom Sets (one for offscreen -> mip0, then mipN -> mipN+1)
        m_BloomDescriptorSets.resize(m_MipLevels + 1);
        std::vector<VkDescriptorSetLayout> bloomLayouts(m_MipLevels + 1, m_BloomSetLayout);

        VkDescriptorSetAllocateInfo bloomAllocInfo{};
        bloomAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        bloomAllocInfo.descriptorPool = m_DescriptorPool;
        bloomAllocInfo.descriptorSetCount = static_cast<uint32_t>(bloomLayouts.size());
        bloomAllocInfo.pSetLayouts = bloomLayouts.data();

        if (vkAllocateDescriptorSets(device, &bloomAllocInfo, m_BloomDescriptorSets.data()) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to allocate bloom descriptor sets!");
        }

        std::vector<VkWriteDescriptorSet> bloomWrites;
        std::vector<VkDescriptorImageInfo> bloomImageInfos(m_MipLevels + 1);

        for (uint32_t i = 0; i < m_MipLevels + 1; i++) {
            bloomImageInfos[i].sampler = m_ColorSampler;
            
            if (i == 0) {
                bloomImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                bloomImageInfos[i].imageView = m_OffscreenColorView; // Offscreen to Mip 0
            } else {
                bloomImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                bloomImageInfos[i].imageView = m_BloomMips[i - 1].view; // Mip i-1 to Mip i
            }

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_BloomDescriptorSets[i];
            write.dstBinding = 0;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &bloomImageInfos[i];
            
            bloomWrites.push_back(write);
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(bloomWrites.size()), bloomWrites.data(), 0, nullptr);
    }
    
    void VulkanPostProcessor::RenderBloom(VkCommandBuffer commandBuffer) {
        // We transition the offscreen color image from Color_Attachment to Shader_Read optimally
        // But for bloom, it is already laid out. Wait, in RenderPass, finalLayout is SHADER_READ_ONLY_OPTIMAL for offscreen.
        // So after Offscreen pass ends, it's ready to be read.

        // --- 1. Bloom Downsample ---
        m_DownsamplePipeline->Bind(commandBuffer);

        for (uint32_t i = 0; i < m_MipLevels; i++) {
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_BloomRenderPass;
            renderPassInfo.framebuffer = m_BloomFramebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {m_BloomMips[i].width, m_BloomMips[i].height};
            
            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(m_BloomMips[i].width);
            viewport.height = static_cast<float>(m_BloomMips[i].height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = {m_BloomMips[i].width, m_BloomMips[i].height};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            // Bind descriptor set (reads from i-1)
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DownsamplePipeline->GetLayout(), 0, 1, &m_BloomDescriptorSets[i], 0, nullptr);

            struct DownPush {
                float resX;
                float resY;
                float threshold;
            } downPush;

            if (i == 0) {
                downPush.resX = static_cast<float>(m_Width);
                downPush.resY = static_cast<float>(m_Height);
                downPush.threshold = bloomThreshold;
            } else {
                downPush.resX = static_cast<float>(m_BloomMips[i - 1].width);
                downPush.resY = static_cast<float>(m_BloomMips[i - 1].height);
                downPush.threshold = 0.0f;
            }

            vkCmdPushConstants(commandBuffer, m_DownsamplePipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DownPush), &downPush);

            // Draw full screen triangle (3 vertices)
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffer);

            // Need image memory barrier here to transition mip 'i' from GENERAL/COLOR_ATTACHMENT to SHADER_READ_ONLY_OPTIMAL for the next pass
            // For simplicity in this engine, our bloom renderpass finalLayout is GENERAL, and loadOp is LOAD.
            // In Vulkan, GENERAL can be read from and written to simultaneously by shaders if synchronized.
        }

        // --- 2. Bloom Upsample ---
        m_UpsamplePipeline->Bind(commandBuffer);

        for (int i = m_MipLevels - 1; i > 0; i--) {
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_BloomRenderPass;
            renderPassInfo.framebuffer = m_BloomFramebuffers[i - 1];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {m_BloomMips[i - 1].width, m_BloomMips[i - 1].height};
            
            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(m_BloomMips[i - 1].width);
            viewport.height = static_cast<float>(m_BloomMips[i - 1].height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = {m_BloomMips[i - 1].width, m_BloomMips[i - 1].height};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            // Bind descriptor set (reads from i)
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_UpsamplePipeline->GetLayout(), 0, 1, &m_BloomDescriptorSets[i], 0, nullptr);

            float filterRad = bloomFilterRadius;
            vkCmdPushConstants(commandBuffer, m_UpsamplePipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &filterRad);

            vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffer);
        }
    }

    void VulkanPostProcessor::RenderFinal(VkCommandBuffer commandBuffer, VkDescriptorSet globalSet, uint32_t currentFrame) {
        // --- 3. Final Post Process Pass ---
        // Assume Swapchain RenderPass has ALREADY begun in VulkanRenderer!
        
        m_PostProcessPipeline->Bind(commandBuffer);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_Width);
        viewport.height = static_cast<float>(m_Height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {m_Width, m_Height};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PostProcessPipeline->GetLayout(), 0, 1, &m_PostProcessDescriptorSet, 0, nullptr);

        struct PostPush {
            float exposure;
            float bloomStrength;
        } postPush;
        postPush.exposure = exposure;
        postPush.bloomStrength = bloomStrength;

        vkCmdPushConstants(commandBuffer, m_PostProcessPipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PostPush), &postPush);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }
    
} // namespace VECTOR
