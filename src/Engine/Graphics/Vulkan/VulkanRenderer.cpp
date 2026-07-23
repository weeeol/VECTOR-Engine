#include "Engine/Graphics/Vulkan/VulkanRenderer.hpp"
#include "Engine/Graphics/Vulkan/VulkanBuffer.hpp"
#include <SDL3/SDL_vulkan.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include "Engine/Graphics/Vulkan/VulkanMesh.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "Engine/Graphics/Vulkan/VulkanUniformBuffer.hpp"
#include "Engine/Graphics/Vulkan/VulkanDescriptorManager.hpp"
#include "Engine/Graphics/Vulkan/VulkanShadowPass.hpp"
#include "Engine/Graphics/Vulkan/VulkanCubemap.hpp"

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

        m_DescriptorManager = std::make_unique<VulkanDescriptorManager>(m_Context.get(), m_FramesInFlight);
        m_DescriptorManager->Initialize();

        m_ShadowPass = std::make_unique<VulkanShadowPass>(m_Context.get());
        m_ShadowPass->Initialize(m_DescriptorManager->GetPipelineLayout());

        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
        
        CreateUniformBuffers();
        CreateDescriptorSets();

        m_PostProcessor = std::make_unique<VulkanPostProcessor>(width, height, m_Swapchain->GetRenderPass());

        VECTOR_LOG_INFO("Loading Shaders...");
        ResourceManager::Get().LoadShader("Default3D", "assets/engine/shaders/vulkan/main3D.vert.spv", "assets/engine/shaders/vulkan/main3D.frag.spv");

        PipelineConfigInfo pipelineConfig{};
        VulkanPipeline::DefaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = m_PostProcessor->GetOffscreenRenderPass();
        pipelineConfig.pipelineLayout = m_DescriptorManager->GetPipelineLayout();
        
        m_Pipeline = std::make_unique<VulkanPipeline>("assets/engine/shaders/vulkan/main3D.vert.spv", "assets/engine/shaders/vulkan/main3D.frag.spv", pipelineConfig);
        
        PipelineConfigInfo wireframeConfig = pipelineConfig;
        wireframeConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
        m_WireframePipeline = std::make_unique<VulkanPipeline>("assets/engine/shaders/vulkan/main3D.vert.spv", "assets/engine/shaders/vulkan/main3D.frag.spv", wireframeConfig);

        PipelineConfigInfo skyboxConfig = pipelineConfig;
        skyboxConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        skyboxConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
        skyboxConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
        m_SkyboxPipeline = std::make_unique<VulkanPipeline>("assets/engine/shaders/vulkan/skybox.vert.spv", "assets/engine/shaders/vulkan/skybox.frag.spv", skyboxConfig);

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

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        m_GameFont = io.Fonts->AddFontFromFileTTF("assets/font.ttf", 48.0f);

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
            
            if (m_PostProcessor) m_PostProcessor.reset();
            m_Pipeline.reset();
            m_WireframePipeline.reset();
            m_SkyboxPipeline.reset();
            
            m_PerFrameUBOs.clear();
            m_LightUBOs.clear();
            m_ObjectDataPool.clear();
            m_DummyObjectUBO.reset();

            if (m_ShadowPass) m_ShadowPass.reset();
            if (m_DescriptorManager) m_DescriptorManager.reset();

            vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
            vkDestroyCommandPool(device, m_CommandPool, nullptr);
        }

        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        m_DummyTexture.reset();
        m_Swapchain.reset();
        m_Context.reset(); // Shutdown happens in destructor

        if (m_Window) {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    void VulkanRenderer::BeginFrame() {
        if (m_FrameStarted) return;
        m_ObjectDataIndex = 0;
        m_RenderQueue.clear();
        m_DrawCallCount = 0;
        
        VkDevice device = m_Context->GetDevice();
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
        
        m_FrameStarted = true;
    }

    void VulkanRenderer::Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        // Convert from sRGB (0-255) to Linear space (0.0-1.0) because the post-processor will gamma correct it back.
        float linR = std::pow(r / 255.0f, 2.2f);
        float linG = std::pow(g / 255.0f, 2.2f);
        float linB = std::pow(b / 255.0f, 2.2f);
        float linA = a / 255.0f; // Alpha usually stays linear

        if (!m_MainPassActive) {
            m_ClearColor = glm::vec4(linR, linG, linB, linA);
            return;
        }

        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
        VkClearAttachment clearColor{};
        clearColor.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clearColor.colorAttachment = 0;
        clearColor.clearValue.color = {{linR, linG, linB, linA}};
        
        VkClearRect clearRect{};
        clearRect.rect.offset = {0, 0};
        clearRect.rect.extent = m_Swapchain->GetExtent();
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = 1;
        
        vkCmdClearAttachments(commandBuffer, 1, &clearColor, 1, &clearRect);
    }

    void VulkanRenderer::Present() {
        if (!m_FrameStarted) {
            BeginFrame();
        }
        if (!m_MainPassActive) {
            BeginMainPass();
        }
        
        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
        
        // 1. End the offscreen main pass
        vkCmdEndRenderPass(commandBuffer);
        m_MainPassActive = false;

        // 2. Render Bloom (does its own RenderPasses)
        m_PostProcessor->RenderBloom(commandBuffer);

        // 3. Begin Swapchain Render Pass for Post Processing & ImGui
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_Swapchain->GetRenderPass();
        renderPassInfo.framebuffer = m_Swapchain->GetFramebuffer(m_ImageIndex);
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_Swapchain->GetExtent();
        
        std::vector<VkClearValue> clearValues(2);
        clearValues[0].color = {{m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a}};
        clearValues[1].depthStencil = {1.0f, 0};
        
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // 4. Render Post Processing Final Pass
        m_PostProcessor->RenderFinal(commandBuffer, m_GlobalDescriptorSets[m_CurrentFrame], m_CurrentFrame);

        // 5. Render ImGui
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        // 5. End Swapchain Render Pass
        vkCmdEndRenderPass(commandBuffer);
        m_MainPassActive = false;
        
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
            VECTOR_LOG_ERROR("Failed to present swap chain image!");
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
        if (m_PostProcessor) {
            m_PostProcessor->Recreate(width, height);
        }
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

    void VulkanRenderer::CreateUniformBuffers() {
        m_PerFrameUBOs.resize(m_FramesInFlight);
        m_LightUBOs.resize(m_FramesInFlight);
        
        for (size_t i = 0; i < m_FramesInFlight; i++) {
            m_PerFrameUBOs[i] = UniformBuffer::Create(static_cast<uint32_t>(sizeof(PerFrameData)), 0);
            m_LightUBOs[i] = UniformBuffer::Create(static_cast<uint32_t>(sizeof(LightUBOData)), 1);
        }
    }

    void VulkanRenderer::CreateDescriptorSets() {
        VkDevice device = m_Context->GetDevice();
        m_GlobalDescriptorSets = m_DescriptorManager->AllocateGlobalSets();

        for (size_t i = 0; i < m_FramesInFlight; i++) {
            auto vulkanUBO = dynamic_cast<VulkanUniformBuffer*>(m_PerFrameUBOs[i].get());
            if (!vulkanUBO) continue;
            VkDescriptorBufferInfo perFrameBufferInfo{};
            perFrameBufferInfo.buffer = vulkanUBO->GetBuffer();
            perFrameBufferInfo.offset = 0;
            perFrameBufferInfo.range = sizeof(PerFrameData);

            VkDescriptorBufferInfo lightBufferInfo{};
            auto lightVulkanUBO = dynamic_cast<VulkanUniformBuffer*>(m_LightUBOs[i].get());
            if (lightVulkanUBO) {
                lightBufferInfo.buffer = lightVulkanUBO->GetBuffer();
                lightBufferInfo.offset = 0;
                lightBufferInfo.range = sizeof(LightUBOData);
            }

            VkDescriptorImageInfo shadowImageInfo{};
            shadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            shadowImageInfo.imageView = m_ShadowPass->GetImageView();
            shadowImageInfo.sampler = m_ShadowPass->GetSampler();

            std::vector<VkWriteDescriptorSet> descriptorWrites;
            
            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_GlobalDescriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &perFrameBufferInfo;
            descriptorWrites.push_back(descriptorWrite);

            if (lightVulkanUBO) {
                VkWriteDescriptorSet lightDescriptorWrite{};
                lightDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                lightDescriptorWrite.dstSet = m_GlobalDescriptorSets[i];
                lightDescriptorWrite.dstBinding = 1;
                lightDescriptorWrite.dstArrayElement = 0;
                lightDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                lightDescriptorWrite.descriptorCount = 1;
                lightDescriptorWrite.pBufferInfo = &lightBufferInfo;
                descriptorWrites.push_back(lightDescriptorWrite);
            }

            if (m_ShadowPass->GetImageView() && m_ShadowPass->GetSampler()) {
                VkWriteDescriptorSet shadowDescriptorWrite{};
                shadowDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                shadowDescriptorWrite.dstSet = m_GlobalDescriptorSets[i];
                shadowDescriptorWrite.dstBinding = 2;
                shadowDescriptorWrite.dstArrayElement = 0;
                shadowDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                shadowDescriptorWrite.descriptorCount = 1;
                shadowDescriptorWrite.pImageInfo = &shadowImageInfo;
                descriptorWrites.push_back(shadowDescriptorWrite);
            }

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        // Create Dummy Texture for Set 1
        uint8_t whitePixel[] = {255, 255, 255, 255};
        m_DummyTexture = std::make_unique<VulkanTexture2D>(1, 1, whitePixel, 4);

        m_DummyMaterialDescriptorSet = m_DescriptorManager->AllocateMaterialSet();
        if (m_DummyMaterialDescriptorSet != VK_NULL_HANDLE) {
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

        // Create Dummy Object Set for Set 2 (Static Meshes)
        m_DummyObjectUBO = std::make_unique<VulkanUniformBuffer>(sizeof(glm::mat4) * 100, 0);
        std::vector<glm::mat4> dummyBones(100, glm::mat4(1.0f));
        m_DummyObjectUBO->SetData(dummyBones.data(), sizeof(glm::mat4) * 100);

        m_DummyObjectSet = m_DescriptorManager->AllocateObjectSet();
        if (m_DummyObjectSet != VK_NULL_HANDLE) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_DummyObjectUBO->GetBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(glm::mat4) * 100;

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_DummyObjectSet;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        } else {
            VECTOR_LOG_ERROR("Failed to allocate dummy object descriptor set!");
        }
    }

    void VulkanRenderer::SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) {
        if (!m_FrameStarted) BeginFrame();

        PerFrameData data{};
        data.view = view;
        data.projection = projection;
        // The Y axis is flipped in Vulkan clip space compared to OpenGL
        data.projection[1][1] *= -1.0f;
        
        glm::vec3 sunDir = glm::normalize(glm::vec3(-0.2f, 1.0f, 0.3f));
        glm::vec3 lightPos = sunDir * 80.0f;
        glm::mat4 lightProjection = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 1.0f, 150.0f);
        // Flip Y for Vulkan projection
        lightProjection[1][1] *= -1.0f;
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        m_LightSpaceMatrix = lightProjection * lightView;
        
        data.lightSpaceMatrix = m_LightSpaceMatrix;
        data.viewPos = glm::vec4(viewPos, 1.0f);
        
        // Fill some default directional light data so scene isn't completely black
        data.sunDir = glm::vec4(sunDir, 0.0f);
        data.sunColor = glm::vec4(1.0f);

        m_PerFrameUBOs[m_CurrentFrame]->SetData(&data, sizeof(PerFrameData), 0);
    }

    void VulkanRenderer::SubmitMesh(const Mesh* mesh, const Material* material, const glm::mat4& model, const std::vector<glm::mat4>* boneTransforms) {
        if (!m_FrameStarted) BeginFrame();
        
        RenderCommand cmd;
        cmd.mesh = mesh;
        cmd.material = material;
        cmd.model = model;
        cmd.boneTransforms = boneTransforms;

        if (boneTransforms && !boneTransforms->empty()) {
            if (m_ObjectDataIndex >= m_ObjectDataPool.size()) {
                ObjectData data;
                data.ubo = std::make_unique<VulkanUniformBuffer>(sizeof(glm::mat4) * 100, 0);
                data.descriptorSet = m_DescriptorManager->AllocateObjectSet();
                
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = data.ubo->GetBuffer();
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(glm::mat4) * 100;
                
                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = data.descriptorSet;
                descriptorWrite.dstBinding = 0;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &bufferInfo;
                
                vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &descriptorWrite, 0, nullptr);
                
                m_ObjectDataPool.push_back(std::move(data));
            }
            
            m_ObjectDataPool[m_ObjectDataIndex].ubo->SetData(boneTransforms->data(), std::min((int)boneTransforms->size(), 100) * sizeof(glm::mat4));
            cmd.objectDescriptorSet = m_ObjectDataPool[m_ObjectDataIndex].descriptorSet;
            m_ObjectDataIndex++;
        } else {
            cmd.objectDescriptorSet = m_DummyObjectSet;
        }

        m_RenderQueue.push_back(std::move(cmd));
    }

    void VulkanRenderer::SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) {
        if (m_LightData.numPointLights < MAX_POINT_LIGHTS) {
            int idx = m_LightData.numPointLights++;
            m_LightData.pointLights[idx].positionAndRadius = glm::vec4(position, radius);
            m_LightData.pointLights[idx].colorAndIntensity = glm::vec4(color, intensity);
        }
    }

    void VulkanRenderer::SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) {
        m_LightData.dirLight.direction = glm::vec4(glm::normalize(direction), 0.0f);
        m_LightData.dirLight.colorAndIntensity = glm::vec4(color, intensity);
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
            m_GameFont ? m_GameFont : ImGui::GetFont(),
            (float)fontSize,
            ImVec2((float)x, (float)y),
            ImColor(color.r, color.g, color.b, color.a),
            text.c_str()
        );
    }

    void VulkanRenderer::EndUI() {
        // UI rendering handled by ImGui
    }

    void VulkanRenderer::SubmitSkybox(VulkanCubemap* cubemap) {
        m_CurrentSkybox = cubemap;
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
        BeginFrame();
        if (!m_FrameStarted) return;
        
        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
        m_ShadowPass->BeginPass(commandBuffer);
    }

    void VulkanRenderer::FlushShadowPass() {
        if (!m_FrameStarted) return;
        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
        
        m_ShadowPass->GetDepthPipeline()->Bind(commandBuffer);
        
        for (const auto& cmd : m_RenderQueue) {
            VkDescriptorSet sets[] = { m_GlobalDescriptorSets[m_CurrentFrame], m_DummyMaterialDescriptorSet, cmd.objectDescriptorSet };
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DescriptorManager->GetPipelineLayout(), 0, 3, sets, 0, nullptr);
            
            struct PushConstants {
                glm::mat4 model;
            } pc;
            pc.model = cmd.model;
            
            vkCmdPushConstants(commandBuffer, m_DescriptorManager->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &pc);
            
            if (cmd.mesh) {
                auto vulkanMesh = dynamic_cast<const VulkanMesh*>(cmd.mesh);
                if (vulkanMesh) {
                    VkBuffer vertexBuffers[] = { vulkanMesh->GetVertexBuffer()->GetBuffer() };
                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                    vkCmdBindIndexBuffer(commandBuffer, vulkanMesh->GetIndexBuffer()->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(commandBuffer, vulkanMesh->GetIndexCount(), 1, 0, 0, 0);
                    m_DrawCallCount++;
                }
            }
        }
        
        m_ShadowPass->EndPass(commandBuffer);
    }

    void VulkanRenderer::BeginMainPass() {
        if (m_MainPassActive) return;
        
        BeginFrame();
        if (!m_FrameStarted) return;
        
        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
        
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_PostProcessor->GetOffscreenRenderPass();
        renderPassInfo.framebuffer = m_PostProcessor->GetOffscreenFramebuffer();
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_Swapchain->GetExtent();
        
        std::vector<VkClearValue> clearValues(2);
        clearValues[0].color = {{m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a}};
        clearValues[1].depthStencil = {1.0f, 0};
        
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_MainPassActive = true;
        
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
    }

    void VulkanRenderer::FlushMainPass() {
        if (!m_FrameStarted) return;
        
        // Update Light UBO before drawing
        m_LightUBOs[m_CurrentFrame]->SetData(&m_LightData, sizeof(LightUBOData), 0);
        m_LightData.numPointLights = 0;
        
        if (m_RenderQueue.empty()) return;

        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];

        // Bind Pipeline
        if (m_IsWireframe) {
            m_WireframePipeline->Bind(commandBuffer);
        } else {
            m_Pipeline->Bind(commandBuffer);
        }

        for (const auto& cmd : m_RenderQueue) {
            VkDescriptorSet materialSet = m_DummyMaterialDescriptorSet;
            if (cmd.material) {
                materialSet = GetOrCreateMaterialDescriptorSet(cmd.material);
            }

            VkDescriptorSet sets[] = { m_GlobalDescriptorSets[m_CurrentFrame], materialSet, cmd.objectDescriptorSet };
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DescriptorManager->GetPipelineLayout(), 0, 3, sets, 0, nullptr);
            // Material Push Constants
            struct PushConstants {
                glm::mat4 model;
                glm::vec4 color;
                uint32_t hasAlbedoMap;
                uint32_t hasNormalMap;
                uint32_t hasMetallicRoughnessMap;
                uint32_t hasAOMap;
                float metallicFactor;
                float roughnessFactor;
                uint32_t isUnlit;
                uint32_t _padding;
            } pc;

            pc.model = cmd.model;
            
            if (cmd.material) {
                pc.color = cmd.material->albedoColor;
                pc.hasAlbedoMap = cmd.material->albedoTexture ? 1 : 0;
                pc.hasNormalMap = cmd.material->normalTexture ? 1 : 0;
                pc.hasMetallicRoughnessMap = cmd.material->metallicRoughnessTexture ? 1 : 0;
                pc.hasAOMap = cmd.material->aoTexture ? 1 : 0;
                pc.metallicFactor = cmd.material->metallic;
                pc.roughnessFactor = cmd.material->roughness;
                pc.isUnlit = cmd.material->isUnlit ? 1 : 0;
            } else {
                pc.color = glm::vec4(1.0f);
                pc.hasAlbedoMap = 0;
                pc.hasNormalMap = 0;
                pc.hasMetallicRoughnessMap = 0;
                pc.hasAOMap = 0;
                pc.metallicFactor = 0.0f;
                pc.roughnessFactor = 0.5f;
                pc.isUnlit = 0;
            }

            vkCmdPushConstants(commandBuffer, m_DescriptorManager->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);

            // Bind Mesh Buffers and Draw
            auto vulkanMesh = dynamic_cast<const VulkanMesh*>(cmd.mesh);
            if (vulkanMesh) {
                VkBuffer vertexBuffers[] = { vulkanMesh->GetVertexBuffer()->GetBuffer() };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                
                vkCmdBindIndexBuffer(commandBuffer, vulkanMesh->GetIndexBuffer()->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
                
                vkCmdDrawIndexed(commandBuffer, vulkanMesh->GetIndexCount(), 1, 0, 0, 0);
                m_DrawCallCount++;
            }
        }

        if (m_CurrentSkybox && m_SkyboxPipeline) {
            m_SkyboxPipeline->Bind(commandBuffer);
            
            // Allocate a descriptor set for the skybox if not done or use a cached one.
            // For simplicity, we can reuse the dummy material set and update it or allocate a new one.
            // But allocating every frame isn't great. Let's just create a new one every frame for now, or use m_SkyboxDescriptorSet.
            if (m_SkyboxDescriptorSet == VK_NULL_HANDLE) {
                m_SkyboxDescriptorSet = m_DescriptorManager->AllocateMaterialSet();
                if (m_SkyboxDescriptorSet != VK_NULL_HANDLE) {
                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfo.imageView = m_CurrentSkybox->GetImageView();
                    imageInfo.sampler = m_CurrentSkybox->GetSampler();

                    VkWriteDescriptorSet descriptorWrite{};
                    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrite.dstSet = m_SkyboxDescriptorSet;
                    descriptorWrite.dstBinding = 0;
                    descriptorWrite.dstArrayElement = 0;
                    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    descriptorWrite.descriptorCount = 1;
                    descriptorWrite.pImageInfo = &imageInfo;

                    vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &descriptorWrite, 0, nullptr);
                }
            }
            
            if (m_SkyboxDescriptorSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DescriptorManager->GetPipelineLayout(), 1, 1, &m_SkyboxDescriptorSet, 0, nullptr);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DescriptorManager->GetPipelineLayout(), 0, 1, &m_GlobalDescriptorSets[m_CurrentFrame], 0, nullptr);
                
                // Draw 36 vertices without vertex buffer
                vkCmdDraw(commandBuffer, 36, 1, 0, 0);
            }
        }

        m_RenderQueue.clear();
        m_CurrentSkybox = nullptr;
    }

    void VulkanRenderer::EndPostProcessPass() {
    }

    void VulkanRenderer::SetFullscreen(bool fullscreen, bool borderless) {
        if (m_Window) {
            SDL_SetWindowFullscreen(m_Window, fullscreen);
        }
    }

    VkDescriptorSet VulkanRenderer::GetOrCreateMaterialDescriptorSet(const Material* material) {
        if (!material) return m_DummyMaterialDescriptorSet;

        MaterialTextures matTex = {
            material->albedoTexture.get(),
            material->normalTexture.get(),
            material->metallicRoughnessTexture.get(),
            material->aoTexture.get()
        };

        auto it = m_MaterialDescriptorSets.find(matTex);
        if (it != m_MaterialDescriptorSets.end()) {
            return it->second;
        }

        VkDevice device = m_Context->GetDevice();
        VkDescriptorSet newSet = m_DescriptorManager->AllocateMaterialSet();
        
        if (newSet != VK_NULL_HANDLE) {
            std::vector<VkWriteDescriptorSet> writes;
            std::vector<VkDescriptorImageInfo> imageInfos(4);

            const Texture2D* texArray[4] = { matTex.albedo, matTex.normal, matTex.mr, matTex.ao };

            for (uint32_t i = 0; i < 4; ++i) {
                auto vulkanTex = dynamic_cast<const VulkanTexture2D*>(texArray[i]);
                if (!vulkanTex) vulkanTex = m_DummyTexture.get();

                imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos[i].imageView = vulkanTex->GetImageView();
                imageInfos[i].sampler = vulkanTex->GetSampler();

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = newSet;
                descriptorWrite.dstBinding = i;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pImageInfo = &imageInfos[i];
                writes.push_back(descriptorWrite);
            }

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
            m_MaterialDescriptorSets[matTex] = newSet;
            return newSet;
        }

        VECTOR_LOG_ERROR("Failed to allocate descriptor set for material!");
        return m_DummyMaterialDescriptorSet;
    }


} // namespace VECTOR
