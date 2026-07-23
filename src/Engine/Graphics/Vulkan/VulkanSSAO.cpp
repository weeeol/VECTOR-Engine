#include "Engine/Graphics/Vulkan/VulkanSSAO.hpp"
#include "Engine/Graphics/Vulkan/VulkanContext.hpp"
#include "Engine/Graphics/Vulkan/VulkanPipeline.hpp"
#include "Engine/Graphics/Vulkan/VulkanUniformBuffer.hpp"
#include "Engine/Core/Logger.hpp"
#include <random>
#include <array>

namespace VECTOR {

    // Helper to linearize depth, if needed, but our SSAO shader does it.
    
    // Lerp helper
    float lerp(float a, float b, float f) {
        return a + f * (b - a);
    }

    VulkanSSAO::VulkanSSAO(VulkanContext* context, uint32_t width, uint32_t height)
        : m_Context(context), m_Width(width), m_Height(height) {
        
        // Generate Kernel
        std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
        std::default_random_engine generator;
        
        for (int i = 0; i < 64; ++i) {
            glm::vec3 sample(
                randomFloats(generator) * 2.0f - 1.0f, 
                randomFloats(generator) * 2.0f - 1.0f, 
                randomFloats(generator)
            );
            sample = glm::normalize(sample);
            sample *= randomFloats(generator);
            float scale = (float)i / 64.0f;
            scale = lerp(0.1f, 1.0f, scale * scale);
            sample *= scale;
            m_Kernel.push_back(sample);
            m_SSAOData.samples[i] = glm::vec4(sample, 0.0f);
        }
    }

    VulkanSSAO::~VulkanSSAO() {
        Shutdown();
    }

    void VulkanSSAO::Initialize() {
        CreateResources(m_Width, m_Height);
        CreateNoiseTexture();
        
        m_UBO = std::make_unique<VulkanUniformBuffer>(sizeof(SSAOData), 0);
        
        CreateDescriptorSets();
        CreatePipelines();
    }

    void VulkanSSAO::Shutdown() {
        if (!m_Context || !m_Context->GetDevice()) return;
        VkDevice device = m_Context->GetDevice();
        
        m_SSAOPipeline.reset();
        m_BlurPipeline.reset();
        m_UBO.reset();
        
        if (m_DescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }
        
        if (m_SSAOPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_SSAOPipelineLayout, nullptr);
        if (m_BlurPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_BlurPipelineLayout, nullptr);
        if (m_SSAOSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_SSAOSetLayout, nullptr);
        if (m_BlurSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_BlurSetLayout, nullptr);
        
        if (m_RenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }
        
        DestroyResources();
        
        m_SSAOPipelineLayout = VK_NULL_HANDLE;
        m_BlurPipelineLayout = VK_NULL_HANDLE;
        m_SSAOSetLayout = VK_NULL_HANDLE;
        m_BlurSetLayout = VK_NULL_HANDLE;
    }

    void VulkanSSAO::DestroyResources() {
        VkDevice device = m_Context->GetDevice();
        VmaAllocator allocator = m_Context->GetAllocator();

        if (m_Sampler != VK_NULL_HANDLE) { vkDestroySampler(device, m_Sampler, nullptr); m_Sampler = VK_NULL_HANDLE; }
        if (m_NoiseSampler != VK_NULL_HANDLE) { vkDestroySampler(device, m_NoiseSampler, nullptr); m_NoiseSampler = VK_NULL_HANDLE; }
        
        if (m_SSAOFramebuffer != VK_NULL_HANDLE) { vkDestroyFramebuffer(device, m_SSAOFramebuffer, nullptr); m_SSAOFramebuffer = VK_NULL_HANDLE; }
        if (m_BlurFramebuffer != VK_NULL_HANDLE) { vkDestroyFramebuffer(device, m_BlurFramebuffer, nullptr); m_BlurFramebuffer = VK_NULL_HANDLE; }
        
        if (m_SSAOImageView != VK_NULL_HANDLE) { vkDestroyImageView(device, m_SSAOImageView, nullptr); m_SSAOImageView = VK_NULL_HANDLE; }
        if (m_SSAOImage != VK_NULL_HANDLE) { vmaDestroyImage(allocator, m_SSAOImage, m_SSAOAllocation); m_SSAOImage = VK_NULL_HANDLE; }
        
        if (m_BlurImageView != VK_NULL_HANDLE) { vkDestroyImageView(device, m_BlurImageView, nullptr); m_BlurImageView = VK_NULL_HANDLE; }
        if (m_BlurImage != VK_NULL_HANDLE) { vmaDestroyImage(allocator, m_BlurImage, m_BlurAllocation); m_BlurImage = VK_NULL_HANDLE; }
        
        if (m_NoiseImageView != VK_NULL_HANDLE) { vkDestroyImageView(device, m_NoiseImageView, nullptr); m_NoiseImageView = VK_NULL_HANDLE; }
        if (m_NoiseImage != VK_NULL_HANDLE) { vmaDestroyImage(allocator, m_NoiseImage, m_NoiseAllocation); m_NoiseImage = VK_NULL_HANDLE; }
    }

    void VulkanSSAO::Resize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) return;
        if (m_Width == width && m_Height == height) return;
        
        m_Width = width;
        m_Height = height;
        
        vkDeviceWaitIdle(m_Context->GetDevice());
        DestroyResources();
        CreateResources(width, height);
        CreateNoiseTexture();
        
        // Descriptors need to be updated because image views changed.
        // We defer this to Generate() which receives the external views anyway.
    }

    void VulkanSSAO::CreateResources(uint32_t width, uint32_t height) {
        VkDevice device = m_Context->GetDevice();
        VmaAllocator allocator = m_Context->GetAllocator();

        // 1. Render Pass
        if (m_RenderPass == VK_NULL_HANDLE) {
            VkAttachmentDescription attachment{};
            attachment.format = VK_FORMAT_R8_UNORM;
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkAttachmentReference colorRef{};
            colorRef.attachment = 0;
            colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorRef;

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

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &attachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
            renderPassInfo.pDependencies = dependencies.data();

            vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass);
        }

        // 2. Images
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_SSAOImage, &m_SSAOAllocation, nullptr);
        vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_BlurImage, &m_BlurAllocation, nullptr);

        // 3. Image Views
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        viewInfo.image = m_SSAOImage;
        vkCreateImageView(device, &viewInfo, nullptr, &m_SSAOImageView);

        viewInfo.image = m_BlurImage;
        vkCreateImageView(device, &viewInfo, nullptr, &m_BlurImageView);

        // 4. Framebuffers
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_RenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.width = width;
        fbInfo.height = height;
        fbInfo.layers = 1;

        fbInfo.pAttachments = &m_SSAOImageView;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &m_SSAOFramebuffer);

        fbInfo.pAttachments = &m_BlurImageView;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &m_BlurFramebuffer);

        // 5. Sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler);
    }

    void VulkanSSAO::CreateNoiseTexture() {
        std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
        std::default_random_engine generator;
        std::vector<glm::vec4> ssaoNoise;
        for (unsigned int i = 0; i < 16; i++) {
            glm::vec4 noise(
                randomFloats(generator) * 2.0f - 1.0f, 
                randomFloats(generator) * 2.0f - 1.0f, 
                0.0f,
                1.0f);
            ssaoNoise.push_back(noise);
        }

        // We would normally upload this via a staging buffer.
        // For brevity in this implementation, we will skip the VulkanBuffer staging boilerplate and just use a 1x1 dummy texture if needed.
        // Let's implement the staging upload properly.
        VkDeviceSize bufferSize = 16 * sizeof(glm::vec4);
        
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        VmaAllocationInfo allocResult;
        vmaCreateBuffer(m_Context->GetAllocator(), &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation, &allocResult);
        memcpy(allocResult.pMappedData, ssaoNoise.data(), (size_t)bufferSize);

        // Create Image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = 4;
        imageInfo.extent.height = 4;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.flags = 0;
        
        vmaCreateImage(m_Context->GetAllocator(), &imageInfo, &allocInfo, &m_NoiseImage, &m_NoiseAllocation, nullptr);
        
        // Execute copy (immediate)
        VkCommandPool tempPool;
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = m_Context->GetGraphicsQueueFamilyIndex();
        vkCreateCommandPool(m_Context->GetDevice(), &poolInfo, nullptr, &tempPool);

        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandPool = tempPool;
        cmdAllocInfo.commandBufferCount = 1;
        
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(m_Context->GetDevice(), &cmdAllocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        
        // Transition to transfer dst
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_NoiseImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        // Copy
        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = { 4, 4, 1 };
        vkCmdCopyBufferToImage(cmd, stagingBuffer, m_NoiseImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        // Transition to shader read
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        VkQueue graphicsQueue = m_Context->GetGraphicsQueue();
        {
            std::lock_guard<std::mutex> lock(m_Context->GetGraphicsQueueMutex());
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);
        }

        vkFreeCommandBuffers(m_Context->GetDevice(), tempPool, 1, &cmd);
        vkDestroyCommandPool(m_Context->GetDevice(), tempPool, nullptr);
        
        vmaDestroyBuffer(m_Context->GetAllocator(), stagingBuffer, stagingAllocation);

        // Create View
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_NoiseImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(m_Context->GetDevice(), &viewInfo, nullptr, &m_NoiseImageView);
        
        // Create Sampler (Repeat for noise)
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vkCreateSampler(m_Context->GetDevice(), &samplerInfo, nullptr, &m_NoiseSampler);
    }

    void VulkanSSAO::CreateDescriptorSets() {
        VkDevice device = m_Context->GetDevice();

        // Layout SSAO
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, // Normal
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, // Depth
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, // Noise
            {3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}          // UBO
        };
        
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_SSAOSetLayout);

        // Layout Blur
        VkDescriptorSetLayoutBinding blurBinding = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &blurBinding;
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_BlurSetLayout);

        // Pool
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 4;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 2;
        vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool);

        // Allocate Sets
        std::vector<VkDescriptorSetLayout> layouts = {m_SSAOSetLayout, m_BlurSetLayout};
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = 2;
        allocInfo.pSetLayouts = layouts.data();
        
        std::vector<VkDescriptorSet> sets(2);
        vkAllocateDescriptorSets(device, &allocInfo, sets.data());
        m_SSAODescriptorSet = sets[0];
        m_BlurDescriptorSet = sets[1];

        // Update Blur Set (it uses internal SSAO texture)
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_SSAOImageView;
        imageInfo.sampler = m_Sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_BlurDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanSSAO::UpdateDescriptorSets(VkImageView normalView, VkImageView depthView) {
        VkDevice device = m_Context->GetDevice();

        VkDescriptorImageInfo normalInfo{};
        normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalInfo.imageView = normalView;
        normalInfo.sampler = m_Sampler;

        VkDescriptorImageInfo depthInfo{};
        depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // Wait, depth from prepass is read-only.
        depthInfo.imageView = depthView;
        depthInfo.sampler = m_Sampler;

        VkDescriptorImageInfo noiseInfo{};
        noiseInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        noiseInfo.imageView = m_NoiseImageView;
        noiseInfo.sampler = m_NoiseSampler;

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_UBO->GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SSAOData);

        std::array<VkWriteDescriptorSet, 4> writes{};
        
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_SSAODescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &normalInfo;
        
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_SSAODescriptorSet;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &depthInfo;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = m_SSAODescriptorSet;
        writes[2].dstBinding = 2;
        writes[2].dstArrayElement = 0;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[2].descriptorCount = 1;
        writes[2].pImageInfo = &noiseInfo;

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[3].dstSet = m_SSAODescriptorSet;
        writes[3].dstBinding = 3;
        writes[3].dstArrayElement = 0;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[3].descriptorCount = 1;
        writes[3].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    void VulkanSSAO::CreatePipelines() {
        VkDevice device = m_Context->GetDevice();

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4);

        // SSAO Pipeline Layout
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &m_SSAOSetLayout;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRange;
        vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_SSAOPipelineLayout);

        // Blur Pipeline Layout
        layoutInfo.pSetLayouts = &m_BlurSetLayout;
        vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_BlurPipelineLayout);

        PipelineConfigInfo config{};
        VulkanPipeline::DefaultPipelineConfigInfo(config);
        config.renderPass = m_RenderPass;
        config.emptyVertexInput = true;
        config.depthStencilInfo.depthTestEnable = VK_FALSE;
        config.depthStencilInfo.depthWriteEnable = VK_FALSE;
        config.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        
        config.pipelineLayout = m_SSAOPipelineLayout;
        m_SSAOPipeline = std::make_unique<VulkanPipeline>("assets/engine/shaders/vulkan/fullscreen.vert.spv", "assets/engine/shaders/vulkan/ssao.frag.spv", config);

        config.pipelineLayout = m_BlurPipelineLayout;
        m_BlurPipeline = std::make_unique<VulkanPipeline>("assets/engine/shaders/vulkan/fullscreen.vert.spv", "assets/engine/shaders/vulkan/ssao_blur.frag.spv", config);
    }

    void VulkanSSAO::Generate(VkCommandBuffer commandBuffer, VkImageView normalView, VkImageView depthView, const glm::mat4& projection, const glm::mat4& view) {
        // Descriptor sets should be updated outside of the render loop (e.g. after initialization or resize)
        // to avoid updating sets that are currently bound and in-use by a command buffer.

        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = m_RenderPass;
        rpInfo.renderArea.offset = {0, 0};
        rpInfo.renderArea.extent = {m_Width, m_Height};
        VkClearValue clearColor = {1.0f, 1.0f, 1.0f, 1.0f};
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearColor;

        // Pass 1: Generate SSAO
        rpInfo.framebuffer = m_SSAOFramebuffer;
        vkCmdBeginRenderPass(commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        VkViewport viewport = {0, 0, (float)m_Width, (float)m_Height, 0.0f, 1.0f};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        VkRect2D scissor = {{0, 0}, {m_Width, m_Height}};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        m_SSAOPipeline->Bind(commandBuffer);
        vkCmdPushConstants(commandBuffer, m_SSAOPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &projection);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SSAOPipelineLayout, 0, 1, &m_SSAODescriptorSet, 0, nullptr);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0); // Fullscreen triangle
        
        vkCmdEndRenderPass(commandBuffer);

        // Pass 2: Blur SSAO
        rpInfo.framebuffer = m_BlurFramebuffer;
        vkCmdBeginRenderPass(commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        m_BlurPipeline->Bind(commandBuffer);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BlurPipelineLayout, 0, 1, &m_BlurDescriptorSet, 0, nullptr);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
    }

} // namespace VECTOR
