#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/Vulkan/VulkanContext.hpp"
#include "Engine/Graphics/Vulkan/VulkanSwapchain.hpp"
#include "Engine/Graphics/UniformBufferObject.hpp"
#include "Engine/Graphics/Vulkan/VulkanPipeline.hpp"
#include "Engine/Graphics/Vulkan/VulkanTexture2D.hpp"
#include "Engine/Graphics/Vulkan/VulkanPostProcessor.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <unordered_map>

struct SDL_Window;
struct ImFont;

namespace VECTOR {

    class VulkanRenderer : public Renderer {
    public:
        VulkanRenderer();
        virtual ~VulkanRenderer();

        virtual bool Initialize(const std::string& title, int width, int height) override;
        virtual void Shutdown() override;
        virtual void WaitIdle() override {
            if (m_Context && m_Context->GetDevice()) {
                vkDeviceWaitIdle(m_Context->GetDevice());
                if (!m_CommandBuffers.empty()) {
                    for (auto cmd : m_CommandBuffers) {
                        vkResetCommandBuffer(cmd, 0);
                    }
                }
            }
        }

        virtual void Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) override;
        virtual void Present() override;

        virtual void SetResolution(int width, int height) override;
        virtual void SetFullscreen(bool fullscreen, bool borderless) override;

        virtual void SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) override;

        virtual void SubmitMesh(const Mesh* mesh, const Material* material, const glm::mat4& model) override;

        virtual void SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) override;
        virtual void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) override;

        virtual void BeginUI() override;
        virtual void DrawUIRect(int x, int y, int w, int h, const glm::vec4& color) override;
        virtual void DrawUIText(const std::string& text, int x, int y, const glm::vec4& color, int fontSize = 24) override;
        virtual void EndUI() override;

        virtual void BeginImGuiFrame() override;
        virtual void EndImGuiFrame() override;

        virtual SDL_Window* GetWindow() const override { return m_Window; }

        virtual void BeginShadowPass() override;
        virtual void FlushShadowPass() override;
        virtual void BeginMainPass() override;
        virtual void FlushMainPass() override;
        virtual void EndPostProcessPass() override;

        virtual const glm::mat4& GetLightSpaceMatrix() const override { return m_DummyMatrix; }
        virtual Shader* GetDepthShader() const override { return nullptr; }
        virtual Material* GetDefaultMaterial() const override { return nullptr; }
        virtual void SetUnlitMode(bool unlit) override {}
        
        virtual void SetWireframeMode(bool enabled) override { m_IsWireframe = enabled; }
        virtual std::string GetRendererInfo() const override { return "Vulkan Renderer (Stub)"; }

        virtual uint32_t GetDrawCallCount() const override { return m_DrawCallCount; }

        struct RenderCommand {
            const Mesh* mesh;
            const Material* material;
            glm::mat4 model;
        };

    private:
        std::vector<RenderCommand> m_RenderQueue;
        
        SDL_Window* m_Window = nullptr;
        std::unique_ptr<VulkanContext> m_Context;
        std::unique_ptr<VulkanSwapchain> m_Swapchain;

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        
        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_InFlightFences;

        uint32_t m_CurrentFrame = 0;
        uint32_t m_ImageIndex = 0;
        uint32_t m_FramesInFlight = 0;

        glm::mat4 m_DummyMatrix = glm::mat4(1.0f);
        bool m_FramebufferResized = false;
        bool m_FrameStarted = false;
        bool m_MainPassActive = false;
        glm::vec4 m_ClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        ImFont* m_GameFont = nullptr;
        
        VkDescriptorSetLayout m_GlobalSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_MaterialSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_MainDescriptorPool = VK_NULL_HANDLE;
        
        std::vector<VkDescriptorSet> m_GlobalDescriptorSets;
        std::vector<std::unique_ptr<UniformBuffer>> m_PerFrameUBOs;
        std::vector<std::unique_ptr<UniformBuffer>> m_LightUBOs;
        LightUBOData m_LightData{};
        bool m_IsWireframe = false;
        
        std::unique_ptr<VulkanPipeline> m_Pipeline;
        std::unique_ptr<VulkanPipeline> m_WireframePipeline;
        
        // Shadow Mapping
        const uint32_t SHADOW_MAP_SIZE = 2048;
        VkRenderPass m_ShadowRenderPass = VK_NULL_HANDLE;
        VkImage m_ShadowImage = VK_NULL_HANDLE;
        VmaAllocation m_ShadowImageAllocation = VK_NULL_HANDLE;
        VkImageView m_ShadowImageView = VK_NULL_HANDLE;
        VkSampler m_ShadowSampler = VK_NULL_HANDLE;
        VkFramebuffer m_ShadowFramebuffer = VK_NULL_HANDLE;
        std::unique_ptr<VulkanPipeline> m_DepthPipeline;
        glm::mat4 m_LightSpaceMatrix = glm::mat4(1.0f);
        
        std::unique_ptr<VulkanTexture2D> m_DummyTexture;
        VkDescriptorSet m_DummyMaterialDescriptorSet = VK_NULL_HANDLE;
        
        std::unordered_map<const Texture2D*, VkDescriptorSet> m_TextureDescriptorSets;
        VkDescriptorSet GetOrCreateTextureDescriptorSet(const Texture2D* texture);

        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void CreateDescriptorSetLayouts();
        void CreateUniformBuffers();
        void CreateDescriptorPoolAndSets();
        
        void BeginFrame();

        void RecreateSwapchain();
        void CreatePipelines();
        
        void CreateShadowResources();
        
        std::unique_ptr<VulkanPostProcessor> m_PostProcessor;
        uint32_t m_DrawCallCount = 0;
    };

} // namespace VECTOR
