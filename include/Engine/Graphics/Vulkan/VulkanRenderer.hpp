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
#include <memory>

struct SDL_Window;
struct ImFont;

namespace VECTOR {
    class VulkanDescriptorManager;
    class VulkanShadowPass;
    class VulkanUniformBuffer;

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

        virtual void SubmitMesh(const Mesh* mesh, const Material* material, const glm::mat4& model, const std::vector<glm::mat4>* boneTransforms = nullptr) override;

        virtual void SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) override;
        virtual void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) override;

        virtual void SubmitSkybox(class VulkanCubemap* cubemap) override;

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
            const std::vector<glm::mat4>* boneTransforms = nullptr;
            VkDescriptorSet objectDescriptorSet = VK_NULL_HANDLE;
        };

    private:
        std::vector<RenderCommand> m_RenderQueue;
        
        SDL_Window* m_Window = nullptr;
        std::unique_ptr<VulkanContext> m_Context;
        std::unique_ptr<VulkanSwapchain> m_Swapchain;
        
        std::unique_ptr<VulkanDescriptorManager> m_DescriptorManager;
        std::unique_ptr<VulkanShadowPass> m_ShadowPass;

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
        
        std::vector<VkDescriptorSet> m_GlobalDescriptorSets;
        std::vector<std::unique_ptr<UniformBuffer>> m_PerFrameUBOs;
        std::vector<std::unique_ptr<UniformBuffer>> m_LightUBOs;
        LightUBOData m_LightData{};
        bool m_IsWireframe = false;
        
        std::unique_ptr<VulkanPipeline> m_Pipeline;
        std::unique_ptr<VulkanPipeline> m_WireframePipeline;
        
        // Shadow Mapping
        glm::mat4 m_LightSpaceMatrix = glm::mat4(1.0f);
        
        std::unique_ptr<VulkanPipeline> m_SkyboxPipeline;
        class VulkanCubemap* m_CurrentSkybox = nullptr;
        VkDescriptorSet m_SkyboxDescriptorSet = VK_NULL_HANDLE;
        
        std::unique_ptr<VulkanTexture2D> m_DummyTexture;
        VkDescriptorSet m_DummyMaterialDescriptorSet = VK_NULL_HANDLE;
        
        struct MaterialTextures {
            const Texture2D* albedo;
            const Texture2D* normal;
            const Texture2D* mr;
            const Texture2D* ao;
            bool operator==(const MaterialTextures& o) const {
                return albedo == o.albedo && normal == o.normal && mr == o.mr && ao == o.ao;
            }
        };
        struct MaterialTexturesHash {
            std::size_t operator()(const MaterialTextures& t) const {
                return std::hash<const void*>()(t.albedo) ^ (std::hash<const void*>()(t.normal) << 1) ^
                       (std::hash<const void*>()(t.mr) << 2) ^ (std::hash<const void*>()(t.ao) << 3);
            }
        };

        std::unordered_map<MaterialTextures, VkDescriptorSet, MaterialTexturesHash> m_MaterialDescriptorSets;
        VkDescriptorSet GetOrCreateMaterialDescriptorSet(const Material* material);

        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void CreateUniformBuffers();
        void CreateDescriptorSets();
        
        void BeginFrame();

        void RecreateSwapchain();
        void CreatePipelines();
        
        std::unique_ptr<VulkanPostProcessor> m_PostProcessor;
        uint32_t m_DrawCallCount = 0;

        struct ObjectData {
            std::unique_ptr<VulkanUniformBuffer> ubo;
            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        };
        std::vector<ObjectData> m_ObjectDataPool;
        uint32_t m_ObjectDataIndex = 0;
        
        std::unique_ptr<VulkanUniformBuffer> m_DummyObjectUBO;
        VkDescriptorSet m_DummyObjectSet = VK_NULL_HANDLE;
    };

} // namespace VECTOR
