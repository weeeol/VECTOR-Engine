#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Swapchain.hpp"
#include "Engine/Graphics/DirectX12/DirectX12DescriptorManager.hpp"
#include <directx/d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_dx12.h>
#include "Engine/Graphics/DirectX12/DirectX12Pipeline.hpp"
#include "Engine/Graphics/DirectX12/DirectX12UniformBuffer.hpp"
#include "Engine/Graphics/DirectX12/DirectX12ShadowPass.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Prepass.hpp"
#include "Engine/Graphics/DirectX12/DirectX12SSAO.hpp"
#include "Engine/Graphics/Mesh.hpp"

namespace VECTOR {

    class DirectX12Renderer : public Renderer {
    public:
        DirectX12Renderer();
        virtual ~DirectX12Renderer();

        virtual bool Initialize(const std::string& title, int width, int height) override;
        virtual void Shutdown() override;
        virtual void WaitIdle() override;

        virtual void Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) override;
        virtual void Present() override;

        virtual void SetResolution(int width, int height) override;
        virtual void SetFullscreen(bool fullscreen, bool borderless) override;

        virtual void SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) override;
        virtual void SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) override;
        virtual void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) override;
        virtual void SubmitSkybox(class Cubemap* cubemap) override;

        virtual void BeginUI() override;
        virtual void DrawUIRect(int x, int y, int w, int h, const glm::vec4& color) override;
        virtual void DrawUIText(const std::string& text, int x, int y, const glm::vec4& color, int fontSize = 24) override;
        virtual void EndUI() override;

        virtual void BeginImGuiFrame() override;
        virtual void EndImGuiFrame() override;

        virtual SDL_Window* GetWindow() const override { return m_Window; }

        virtual void BeginShadowPass() override;
        virtual void FlushShadowPass() override;
        
        virtual void BeginPrepass() override;
        virtual void FlushPrepass() override;
        
        virtual void BeginMainPass() override;
        virtual void FlushMainPass() override;
        virtual void EndPostProcessPass() override;

        virtual const glm::mat4& GetLightSpaceMatrix() const override { return m_LightSpaceMatrix; }
        virtual Shader* GetDepthShader() const override { return nullptr; }
        virtual Material* GetDefaultMaterial() const override { return nullptr; }
        virtual void SetUnlitMode(bool unlit) override {}
        
        virtual void SetWireframeMode(bool enabled) override;
        virtual std::string GetRendererInfo() const override { return "DirectX 12 Renderer"; }

        virtual uint32_t GetDrawCallCount() const override { return 0; }

        virtual void SetSSAOEnabled(bool enabled) override { m_SSAOEnabled = enabled; }
        virtual bool IsSSAOEnabled() const override { return m_SSAOEnabled; }

        virtual void SetBloomEnabled(bool enabled) override;
        virtual bool IsBloomEnabled() const override;

        virtual void SetTAAEnabled(bool enabled) override { m_TAAEnabled = enabled; }
        virtual bool IsTAAEnabled() const override { return m_TAAEnabled; }

        // Transition helper utilizing Enhanced Barriers if available, or fallback to legacy
        void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    private:
        void CreateCommandObjects();
        void CreateSyncObjects();
        void MoveToNextFrame();
        void InitImGui();
        void BindObjectAndMaterial(const RenderCommand& cmd, bool bindMaterial);

    private:
        SDL_Window* m_Window = nullptr;
        std::unique_ptr<DirectX12Context> m_Context;
        std::unique_ptr<DirectX12Swapchain> m_Swapchain;
        std::unique_ptr<DirectX12DescriptorManager> m_DescriptorManager;

        static const uint32_t s_FrameCount = 3;
        uint32_t m_FrameIndex = 0;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocators[s_FrameCount];
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> m_CommandList;

        struct ObjectData {
            std::unique_ptr<UniformBuffer> ubo;
        };

        std::vector<ObjectData> m_ObjectDataPool;
        size_t m_ObjectDataIndex = 0;
        
        struct MaterialDataBlock {
            std::unique_ptr<UniformBuffer> ubo;
        };
        std::vector<MaterialDataBlock> m_MaterialDataPool;
        size_t m_MaterialDataIndex = 0;

        std::vector<std::unique_ptr<UniformBuffer>> m_PerFrameUBOs;
        std::vector<std::unique_ptr<UniformBuffer>> m_LightUBOs;
        std::unique_ptr<UniformBuffer> m_DummyObjectUBO;

        struct LightUBOData {
            glm::vec4 dirLightDir;
            glm::vec4 dirLightColor;
            glm::vec4 pointLights[64 * 2]; // PosRad, ColorInt
            int numPointLights = 0;
            glm::vec3 padding;
        } m_LightData;

        std::unique_ptr<DirectX12Pipeline> m_Pipeline;
        std::unique_ptr<DirectX12Pipeline> m_SkyboxPipeline;
        std::unique_ptr<class DirectX12ShadowPass> m_ShadowPass;
        std::unique_ptr<class DirectX12Prepass> m_Prepass;
        std::unique_ptr<class DirectX12SSAO> m_SSAO;
        std::unique_ptr<class DirectX12TAA> m_TAA;
        std::unique_ptr<class DirectX12PostProcessor> m_PostProcessor;
        class DirectX12Cubemap* m_CurrentSkybox = nullptr;
        glm::mat4 m_LightSpaceMatrix = glm::mat4(1.0f);
        
        glm::mat4 m_PreviousView = glm::mat4(1.0f);
        glm::mat4 m_PreviousProjection = glm::mat4(1.0f);
        glm::vec2 m_Jitter = glm::vec2(0.0f);
        glm::vec2 m_PreviousJitter = glm::vec2(0.0f);
        uint32_t m_TAAFrameIndex = 0;

        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        uint64_t m_FenceValues[s_FrameCount] = {0};
        HANDLE m_FenceEvent;
        glm::mat4 m_CurrentView = glm::mat4(1.0f);
        glm::mat4 m_CurrentProjection = glm::mat4(1.0f);
        glm::mat4 m_UnjitteredProjection = glm::mat4(1.0f);
        bool m_FrameStarted = false;
        bool m_WireframeMode = false;
        bool m_SSAOEnabled = true;
        bool m_BloomEnabled = true;
        bool m_TAAEnabled = true;
        ImFont* m_GameFont = nullptr;
        std::unique_ptr<DirectX12Pipeline> m_WireframePipeline;
    };

} // namespace VECTOR
