#include "Engine/Graphics/Vulkan/VulkanRenderer.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include "Engine/Graphics/Vulkan/VulkanMesh.hpp"
#include "Engine/Graphics/Vulkan/VulkanUniformBuffer.hpp"

namespace VECTOR {

    VulkanRenderer::VulkanRenderer() {
    }

    VulkanRenderer::~VulkanRenderer() {
    }

    bool VulkanRenderer::Initialize(const std::string& title, int width, int height) {
        VECTOR_LOG_INFO("VulkanRenderer::Initialize called");
        
        m_Window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        if (!m_Window) {
            VECTOR_LOG_ERROR(std::string("Failed to create SDL window for Vulkan: ") + SDL_GetError());
            return false;
        }

        m_Context = std::make_unique<VulkanContext>();
        if (!m_Context->Initialize(m_Window)) {
            VECTOR_LOG_ERROR("Failed to initialize Vulkan Context!");
            return false;
        }

        m_Swapchain = std::make_unique<VulkanSwapchain>(m_Context.get(), width, height);
        m_FramesInFlight = static_cast<uint32_t>(m_Swapchain->GetImageCount());

        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
        
        CreateDescriptorSetLayouts();
        CreateUniformBuffers();
        CreateDescriptorPoolAndSets();

        VECTOR_LOG_INFO("Loading Shaders...");
        ResourceManager::Get().LoadShader("Default3D", "assets/engine/shaders/vulkan/main3D.vert.spv", "assets/engine/shaders/vulkan/main3D.frag.spv");

        PipelineConfigInfo pipelineConfig{};
        VulkanPipeline::DefaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = m_Swapchain->GetRenderPass();
        pipelineConfig.pipelineLayout = m_PipelineLayout;
        
        m_Pipeline = std::make_unique<VulkanPipeline>("assets/engine/shaders/vulkan/main3D.vert.spv", "assets/engine/shaders/vulkan/main3D.frag.spv", pipelineConfig);

        // Create Descriptor Pool for ImGui
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        if (vkCreateDescriptorPool(m_Context->GetDevice(), &pool_info, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create descriptor pool for ImGui!");
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplSDL3_InitForVulkan(m_Window);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_Context->GetInstance();
        init_info.PhysicalDevice = m_Context->GetPhysicalDevice();
        init_info.Device = m_Context->GetDevice();
        init_info.QueueFamily = m_Context->GetGraphicsQueueFamilyIndex();
        init_info.Queue = m_Context->GetGraphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = m_DescriptorPool;
        init_info.MinImageCount = m_FramesInFlight;
        init_info.ImageCount = m_FramesInFlight;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        
        init_info.PipelineInfoMain.RenderPass = m_Swapchain->GetRenderPass();
        init_info.PipelineInfoMain.Subpass = 0;
        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.UseDynamicRendering = false;
        
        ImGui_ImplVulkan_Init(&init_info);

        return true;
    }

    void VulkanRenderer::Shutdown() {
        VECTOR_LOG_INFO("VulkanRenderer::Shutdown called");
        
        if (m_Context && m_Context->GetDevice()) {
            vkDeviceWaitIdle(m_Context->GetDevice());
            
            ImGui_ImplVulkan_Shutdown();
            
            VkDevice device = m_Context->GetDevice();
            for (size_t i = 0; i < m_FramesInFlight; i++) {
                vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
                vkDestroyFence(device, m_InFlightFences[i], nullptr);
            }
            
            m_DummyTexture.reset();
            m_Pipeline.reset();
            
            m_PerFrameUBOs.clear();
            m_LightUBOs.clear();

            if (m_MainDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, m_MainDescriptorPool, nullptr);
            if (m_PipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
            if (m_GlobalSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_GlobalSetLayout, nullptr);
            if (m_MaterialSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, m_MaterialSetLayout, nullptr);

            vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
            vkDestroyCommandPool(device, m_CommandPool, nullptr);
        }

        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        m_Swapchain.reset();
        m_Context.reset(); // Shutdown happens in destructor

        if (m_Window) {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    void VulkanRenderer::Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        VkDevice device = m_Context->GetDevice();

        if (!m_FrameStarted) {
            vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

            VkResult result = m_Swapchain->AcquireNextImage(m_ImageAvailableSemaphores[m_CurrentFrame], &m_ImageIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                RecreateSwapchain();
                return;
            } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                VECTOR_LOG_ERROR("Failed to acquire swapchain image!");
                return;
            }

            vkResetFences(device, 1, &m_InFlightFences[m_CurrentFrame]);

            VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
            vkResetCommandBuffer(commandBuffer, 0);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
                VECTOR_LOG_ERROR("Failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_Swapchain->GetRenderPass();
            renderPassInfo.framebuffer = m_Swapchain->GetFramebuffer(m_ImageIndex);
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = m_Swapchain->GetExtent();

            std::vector<VkClearValue> clearValues(2);
            clearValues[0].color = {{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f}};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(m_Swapchain->GetExtent().width);
            viewport.height = static_cast<float>(m_Swapchain->GetExtent().height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = m_Swapchain->GetExtent();
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            m_FrameStarted = true;
        } else {
            // Already started, just clear the attachments
            VkClearAttachment clearColor{};
            clearColor.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            clearColor.colorAttachment = 0;
            clearColor.clearValue.color = {{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f}};
            
            VkClearAttachment clearDepth{};
            clearDepth.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            clearDepth.clearValue.depthStencil = {1.0f, 0};

            VkClearAttachment attachments[] = {clearColor, clearDepth};

            VkClearRect clearRect{};
            clearRect.rect.offset = {0, 0};
            clearRect.rect.extent = m_Swapchain->GetExtent();
            clearRect.baseArrayLayer = 0;
            clearRect.layerCount = 1;

            vkCmdClearAttachments(m_CommandBuffers[m_CurrentFrame], 2, attachments, 1, &clearRect);
        }
    }

    void VulkanRenderer::Present() {
        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
        
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        vkCmdEndRenderPass(commandBuffer);
        
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to record command buffer!");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(m_Context->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to submit draw command buffer!");
        }

        VkResult result = m_Swapchain->Present(m_Context->GetGraphicsQueue(), m_ImageIndex, m_RenderFinishedSemaphores[m_CurrentFrame]);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
            m_FramebufferResized = false;
            RecreateSwapchain();
        } else if (result != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to present swapchain image!");
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % m_FramesInFlight;
        m_FrameStarted = false;
    }

    void VulkanRenderer::SetResolution(int width, int height) {
        if (m_Window) {
            SDL_SetWindowSize(m_Window, width, height);
        }
        m_FramebufferResized = true;
    }

    void VulkanRenderer::RecreateSwapchain() {
        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels(m_Window, &width, &height);
        while (width == 0 || height == 0) {
            SDL_GetWindowSizeInPixels(m_Window, &width, &height);
            SDL_WaitEvent(nullptr);
        }

        vkDeviceWaitIdle(m_Context->GetDevice());

        m_Swapchain->Recreate(width, height);
    }

    void VulkanRenderer::CreateCommandPool() {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_Context->GetGraphicsQueueFamilyIndex();

        if (vkCreateCommandPool(m_Context->GetDevice(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create command pool!");
        }
    }

    void VulkanRenderer::CreateCommandBuffers() {
        m_CommandBuffers.resize(m_FramesInFlight);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

        if (vkAllocateCommandBuffers(m_Context->GetDevice(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to allocate command buffers!");
        }
    }

    void VulkanRenderer::CreateSyncObjects() {
        m_ImageAvailableSemaphores.resize(m_FramesInFlight);
        m_RenderFinishedSemaphores.resize(m_FramesInFlight);
        m_InFlightFences.resize(m_FramesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkDevice device = m_Context->GetDevice();
        for (size_t i = 0; i < m_FramesInFlight; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
                VECTOR_LOG_ERROR("Failed to create synchronization objects for a frame!");
            }
        }
    }

    void VulkanRenderer::CreateDescriptorSetLayouts() {
        VkDevice device = m_Context->GetDevice();

        // 1. Global Set Layout (Set 0: UBOs)
        VkDescriptorSetLayoutBinding perFrameBinding{};
        perFrameBinding.binding = 0;
        perFrameBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        perFrameBinding.descriptorCount = 1;
        perFrameBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo globalLayoutInfo{};
        globalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        globalLayoutInfo.bindingCount = 1;
        globalLayoutInfo.pBindings = &perFrameBinding;

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
        pushConstant.size = sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(uint32_t); // model + color + bool
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

    void VulkanRenderer::CreateUniformBuffers() {
        m_PerFrameUBOs.resize(m_FramesInFlight);
        m_LightUBOs.resize(m_FramesInFlight);
        
        for (size_t i = 0; i < m_FramesInFlight; i++) {
            m_PerFrameUBOs[i] = UniformBuffer::Create(static_cast<uint32_t>(sizeof(PerFrameData)), 0);
            m_LightUBOs[i] = UniformBuffer::Create(static_cast<uint32_t>(sizeof(LightUBOData)), 1);
        }
    }

    void VulkanRenderer::CreateDescriptorPoolAndSets() {
        VkDevice device = m_Context->GetDevice();

        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(m_FramesInFlight * 2); // 2 UBOs per frame
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(1024); // max textures

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(m_FramesInFlight + 1024);
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_MainDescriptorPool) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create main descriptor pool!");
        }

        std::vector<VkDescriptorSetLayout> layouts(m_FramesInFlight, m_GlobalSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_MainDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(m_FramesInFlight);
        allocInfo.pSetLayouts = layouts.data();

        m_GlobalDescriptorSets.resize(m_FramesInFlight);
        if (vkAllocateDescriptorSets(device, &allocInfo, m_GlobalDescriptorSets.data()) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to allocate global descriptor sets!");
        }

        for (size_t i = 0; i < m_FramesInFlight; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            auto vulkanUBO = dynamic_cast<VulkanUniformBuffer*>(m_PerFrameUBOs[i].get());
            if (!vulkanUBO) continue;

            bufferInfo.buffer = vulkanUBO->GetBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(PerFrameData);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_GlobalDescriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }

        // Create Dummy Texture for Set 1
        uint8_t whitePixel[] = {255, 255, 255, 255};
        m_DummyTexture = std::make_unique<VulkanTexture2D>(1, 1, whitePixel, 4);

        VkDescriptorSetAllocateInfo dummyAllocInfo{};
        dummyAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dummyAllocInfo.descriptorPool = m_MainDescriptorPool;
        dummyAllocInfo.descriptorSetCount = 1;
        dummyAllocInfo.pSetLayouts = &m_MaterialSetLayout;

        if (vkAllocateDescriptorSets(device, &dummyAllocInfo, &m_DummyMaterialDescriptorSet) == VK_SUCCESS) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_DummyTexture->GetImageView();
            imageInfo.sampler = m_DummyTexture->GetSampler();

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_DummyMaterialDescriptorSet;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        } else {
            VECTOR_LOG_ERROR("Failed to allocate dummy material descriptor set!");
        }
    }

    void VulkanRenderer::SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) {
        if (!m_FrameStarted) return;

        PerFrameData data{};
        data.view = view;
        data.projection = projection;
        // The Y axis is flipped in Vulkan clip space compared to OpenGL
        // glm handles this via GLM_FORCE_DEPTH_ZERO_TO_ONE in our CMakeLists, but we might also need to invert Y in projection.
        // For now, we'll invert Y in projection matrix here to avoid upside down rendering.
        data.projection[1][1] *= -1.0f;
        
        data.lightSpaceMatrix = glm::mat4(1.0f); // Default for now
        data.viewPos = glm::vec4(viewPos, 1.0f);
        
        // Fill some default directional light data so scene isn't completely black
        data.sunDir = glm::vec4(glm::normalize(glm::vec3(0.5f, 1.0f, 0.5f)), 0.0f);
        data.sunColor = glm::vec4(1.0f);

        m_PerFrameUBOs[m_CurrentFrame]->SetData(&data, sizeof(PerFrameData), 0);
    }

    void VulkanRenderer::SubmitMesh(const Mesh* mesh, const Material* material, const glm::mat4& model) {
        if (!m_FrameStarted) return;
        
        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
        
        // Bind Pipeline
        m_Pipeline->Bind(commandBuffer);

        // Bind Global Descriptor Set (Set 0) and Material Set (Set 1)
        VkDescriptorSet sets[] = { m_GlobalDescriptorSets[m_CurrentFrame], m_DummyMaterialDescriptorSet };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 2, sets, 0, nullptr);

        // Material Push Constants
        struct PushConstants {
            glm::mat4 model;
            glm::vec4 albedoColor;
            uint32_t hasDiffuseMap;
        } pc;

        pc.model = model;
        
        if (material) {
            pc.albedoColor = material->albedoColor;
            pc.hasDiffuseMap = (material->albedoTexture != nullptr) ? 1 : 0;
        } else {
            pc.albedoColor = glm::vec4(1.0f);
            pc.hasDiffuseMap = 0;
        }

        vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);

        // Bind Mesh Buffers and Draw
        auto vulkanMesh = dynamic_cast<const VulkanMesh*>(mesh);
        if (vulkanMesh) {
            VkBuffer vertexBuffers[] = { vulkanMesh->GetVertexBuffer()->GetBuffer() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            
            vkCmdBindIndexBuffer(commandBuffer, vulkanMesh->GetIndexBuffer()->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
            
            vkCmdDrawIndexed(commandBuffer, vulkanMesh->GetIndexCount(), 1, 0, 0, 0);
        }
    }

    void VulkanRenderer::SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) {
    }

    void VulkanRenderer::SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) {
    }

    void VulkanRenderer::BeginUI() {
        // UI rendering handled by ImGui
    }

    void VulkanRenderer::DrawUIRect(int x, int y, int w, int h, const glm::vec4& color) {
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2((float)x, (float)y),
            ImVec2((float)(x + w), (float)(y + h)),
            ImColor(color.r, color.g, color.b, color.a)
        );
    }

    void VulkanRenderer::DrawUIText(const std::string& text, int x, int y, const glm::vec4& color, int fontSize) {
        ImGui::GetBackgroundDrawList()->AddText(
            ImGui::GetFont(),
            (float)fontSize,
            ImVec2((float)x, (float)y),
            ImColor(color.r, color.g, color.b, color.a),
            text.c_str()
        );
    }

    void VulkanRenderer::EndUI() {
        // UI rendering handled by ImGui
    }

    void VulkanRenderer::BeginImGuiFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void VulkanRenderer::EndImGuiFrame() {
        ImGui::Render();
    }

    void VulkanRenderer::BeginShadowPass() {
    }

    void VulkanRenderer::FlushShadowPass() {
    }

    void VulkanRenderer::BeginMainPass() {
    }

    void VulkanRenderer::FlushMainPass() {
    }

    void VulkanRenderer::EndPostProcessPass() {
    }

    void VulkanRenderer::SetFullscreen(bool fullscreen, bool borderless) {
        if (m_Window) {
            SDL_SetWindowFullscreen(m_Window, fullscreen);
        }
    }

} // namespace VECTOR
