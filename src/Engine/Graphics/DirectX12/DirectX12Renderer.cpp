#define NOMINMAX
#include "Engine/Graphics/DirectX12/DirectX12Renderer.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Mesh.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Prepass.hpp"
#include "Engine/Graphics/DirectX12/DirectX12SSAO.hpp"
#include "Engine/Graphics/DirectX12/DirectX12TAA.hpp"
#include "Engine/Graphics/DirectX12/DirectX12PostProcessor.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Texture2D.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Cubemap.hpp"
#include <directx/d3dx12.h>
#include "Engine/Core/ResourceManager.hpp"
#include "Engine/Core/Logger.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_dx12.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

namespace VECTOR {

    DirectX12Renderer::DirectX12Renderer() {
    }

    DirectX12Renderer::~DirectX12Renderer() {
        Shutdown();
    }

    bool DirectX12Renderer::Initialize(const std::string& title, int width, int height) {
        // Initialize SDL Window (Assuming it's created externally or here)
        uint32_t windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        m_Window = SDL_CreateWindow(title.c_str(), width, height, windowFlags);
        if (!m_Window) {
            VECTOR_LOG_ERROR("Failed to create SDL Window for DirectX 12");
            return false;
        }

        m_Context = std::make_unique<DirectX12Context>();
        if (!m_Context->Initialize(m_Window)) {
            return false;
        }

        int pixelWidth, pixelHeight;
        SDL_GetWindowSizeInPixels(m_Window, &pixelWidth, &pixelHeight);

        m_Swapchain = std::make_unique<DirectX12Swapchain>(m_Context.get(), m_Window, pixelWidth, pixelHeight);

        CreateCommandObjects();
        CreateSyncObjects();
        m_DescriptorManager = std::make_unique<DirectX12DescriptorManager>();
        m_DescriptorManager->Initialize();
        VECTOR_LOG_INFO("DirectX 12 Descriptor Manager Initialized.");
        
        InitImGui();
        VECTOR_LOG_INFO("ImGui Initialized.");

        // Create UBOs
        for (int i = 0; i < s_FrameCount; ++i) {
            m_PerFrameUBOs.push_back(std::make_unique<DirectX12UniformBuffer>(512, 0)); // Increased for shadow mapping fields
            m_LightUBOs.push_back(std::make_unique<DirectX12UniformBuffer>(static_cast<uint32_t>(sizeof(LightUBOData)), 1));
        }
        // Dummy Object Data
        m_DummyObjectUBO = std::make_unique<DirectX12UniformBuffer>(static_cast<uint32_t>(sizeof(glm::mat4) * 101), 2);

        // m_MaterialDataPool is used for materials

        // Skybox pipeline will be created in SubmitSkybox if needed, or initialized here
        // (will initialize later if needed)
        VECTOR_LOG_INFO("UBOs created.");
        

        // Create Pipeline
        VECTOR_LOG_INFO("Creating DirectX 12 Pipeline...");
        auto shader = ResourceManager::Get().LoadShader("Default3D", "assets/engine/shaders/dx12/main3D.hlsl", "assets/engine/shaders/dx12/main3D.hlsl");
        DirectX12PipelineConfig mainConfig;
        mainConfig.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        m_Pipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(shader.get()), mainConfig);

        DirectX12PipelineConfig wireframeConfig;
        wireframeConfig.wireframe = true;
        wireframeConfig.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        m_WireframePipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(shader.get()), wireframeConfig);

        VECTOR_LOG_INFO("Creating DirectX 12 Skybox Pipeline...");
        auto skyboxShader = ResourceManager::Get().LoadShader("Skybox", "assets/engine/shaders/dx12/skybox.hlsl", "assets/engine/shaders/dx12/skybox.hlsl");
        DirectX12PipelineConfig skyboxConfig;
        skyboxConfig.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        skyboxConfig.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        m_SkyboxPipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(skyboxShader.get()), skyboxConfig);

        VECTOR_LOG_INFO("Initializing DirectX 12 Shadow Pass...");
        m_ShadowPass = std::make_unique<DirectX12ShadowPass>(m_Context.get());
        m_ShadowPass->Initialize();

        m_Prepass = std::make_unique<DirectX12Prepass>(m_Context.get(), pixelWidth, pixelHeight);
        m_Prepass->Initialize();
        
        m_SSAO = std::make_unique<DirectX12SSAO>(m_Context.get(), pixelWidth, pixelHeight);
        m_SSAO->Initialize();

        m_TAA = std::make_unique<DirectX12TAA>(m_Context.get(), pixelWidth, pixelHeight);
        m_TAA->Initialize();

        VECTOR_LOG_INFO("Initializing DirectX 12 PostProcessor...");
        m_PostProcessor = std::make_unique<DirectX12PostProcessor>(m_Context.get(), pixelWidth, pixelHeight);
        m_PostProcessor->Initialize();

        VECTOR_LOG_INFO("DirectX 12 Renderer Initialized.");
        return true;
    }

    void DirectX12Renderer::Shutdown() {
        VECTOR_LOG_INFO("DirectX12Renderer::Shutdown called");

        m_RenderGraph.Clear();

        if (!m_Window) return; // Already shut down

        WaitIdle();
        
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        if (m_FenceEvent) {
            CloseHandle(m_FenceEvent);
            m_FenceEvent = nullptr;
        }

        m_Fence.Reset();
        m_CommandList.Reset();
        for (int i = 0; i < s_FrameCount; ++i) {
            m_CommandAllocators[i].Reset();
        }

        m_PerFrameUBOs.clear();
        m_LightUBOs.clear();
        m_DummyObjectUBO.reset();
        m_MaterialDataPool.clear();
        m_ObjectDataPool.clear();
        
        m_Pipeline.reset();
        m_WireframePipeline.reset();
        m_SkyboxPipeline.reset();
        m_ShadowPass.reset();
        m_Prepass.reset();
        m_SSAO.reset();
        m_TAA.reset();
        m_PostProcessor.reset();
        m_DescriptorManager.reset();

        m_Swapchain.reset();
        m_Context.reset();

        if (m_Window) {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    void DirectX12Renderer::WaitIdle() {
        if (!m_Context || !m_Context->GetGraphicsQueue()) return;

        uint64_t fenceValue = m_FenceValues[m_FrameIndex];
        m_Context->GetGraphicsQueue()->Signal(m_Fence.Get(), fenceValue);

        if (m_Fence->GetCompletedValue() < fenceValue) {
            m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);
            WaitForSingleObject(m_FenceEvent, INFINITE);
        }

        m_FenceValues[m_FrameIndex]++;
    }

    void DirectX12Renderer::CreateCommandObjects() {
        auto device = m_Context->GetDevice();

        for (uint32_t i = 0; i < s_FrameCount; ++i) {
            device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocators[i]));
        }

        device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_CommandList));
        m_CommandList->Close();
    }

    void DirectX12Renderer::CreateSyncObjects() {
        auto device = m_Context->GetDevice();
        
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
        m_FenceValues[0] = 1;

        m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_FenceEvent == nullptr) {
            VECTOR_LOG_ERROR("Failed to create fence event.");
        }
    }

    void DirectX12Renderer::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
        if (stateBefore == stateAfter) return;
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    void DirectX12Renderer::Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {

        uint32_t backBufferIndex = m_Swapchain->AcquireNextImage();
        ID3D12Resource* backBuffer = m_Swapchain->GetBackBuffer(backBufferIndex);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_Swapchain->GetBackBufferRTV(backBufferIndex);
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_Swapchain->GetDepthBufferDSV();

        if (!m_FrameStarted) {
            m_CommandAllocators[m_FrameIndex]->Reset();
            m_CommandList->Reset(m_CommandAllocators[m_FrameIndex].Get(), nullptr);
            TransitionResource(m_CommandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            m_ObjectDataIndex = 0;
            m_MaterialDataIndex = 0;

            m_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

            D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_Swapchain->GetWidth()), static_cast<float>(m_Swapchain->GetHeight()), 0.0f, 1.0f };
            D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_Swapchain->GetWidth()), static_cast<LONG>(m_Swapchain->GetHeight()) };
            m_CommandList->RSSetViewports(1, &viewport);
            m_CommandList->RSSetScissorRects(1, &scissorRect);
            
            ID3D12DescriptorHeap* descriptorHeaps[] = { 
                DirectX12DescriptorManager::Get()->GetSRVHeap(),
                DirectX12DescriptorManager::Get()->GetSamplerHeap()
            };
            m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
            
            m_FrameStarted = true;
        }

        float clearColor[] = { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
        // No longer clearing HDR texture here as it's passed into the RenderPass which can clear it if needed.
        // m_CommandList->ClearRenderTargetView(hdrRTV, clearColor, 0, nullptr);
        
        // Also clear the backbuffer, because if a scene doesn't call EndPostProcessPass, the backbuffer will have garbage
        D3D12_CPU_DESCRIPTOR_HANDLE backbufferRTV = m_Swapchain->GetBackBufferRTV(m_Swapchain->AcquireNextImage());
        m_CommandList->ClearRenderTargetView(backbufferRTV, clearColor, 0, nullptr);
        
        m_CommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    void DirectX12Renderer::Present() {
        if (!m_FrameStarted) {
            VECTOR_LOG_INFO("Present called without Clear, returning.");
            return;
        }

        uint32_t backBufferIndex = m_Swapchain->AcquireNextImage();
        ID3D12Resource* backBuffer = m_Swapchain->GetBackBuffer(backBufferIndex);

        TransitionResource(m_CommandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        m_CommandList->Close();

        ID3D12CommandList* ppCommandLists[] = { m_CommandList.Get() };
        m_Context->GetGraphicsQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        m_Swapchain->Present(false);

        MoveToNextFrame();
        m_FrameStarted = false;
    }

    void DirectX12Renderer::MoveToNextFrame() {
        const uint64_t currentFenceValue = m_FenceValues[m_FrameIndex];
        m_Context->GetGraphicsQueue()->Signal(m_Fence.Get(), currentFenceValue);

        m_FrameIndex = (m_FrameIndex + 1) % s_FrameCount;

        if (m_Fence->GetCompletedValue() < m_FenceValues[m_FrameIndex]) {
            m_Fence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_FenceEvent);
            WaitForSingleObject(m_FenceEvent, INFINITE);
        }

        m_FenceValues[m_FrameIndex] = currentFenceValue + 1;
    }

    void DirectX12Renderer::SetResolution(int width, int height) {
        if (width == 0 || height == 0) return;
        m_RenderGraph.Clear();
        WaitIdle();
        SDL_SetWindowSize(m_Window, width, height);
        SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        
        int pixelWidth, pixelHeight;
        SDL_GetWindowSizeInPixels(m_Window, &pixelWidth, &pixelHeight);

        m_Swapchain->Recreate(pixelWidth, pixelHeight);
        
        if (m_PostProcessor) m_PostProcessor->Recreate(pixelWidth, pixelHeight);
        if (m_Prepass) m_Prepass->Resize(pixelWidth, pixelHeight);
        if (m_SSAO) m_SSAO->Resize(pixelWidth, pixelHeight);
    }

    void DirectX12Renderer::SetFullscreen(bool fullscreen, bool borderless) {
        SDL_SetWindowFullscreen(m_Window, fullscreen);
        if (!fullscreen) {
            SDL_SetWindowBordered(m_Window, !borderless);
        }
    }

    void DirectX12Renderer::InitImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForD3D(m_Window);
        ImGui_ImplDX12_InitInfo init_info = {};
        init_info.Device = m_Context->GetDevice().Get();
        init_info.CommandQueue = m_Context->GetGraphicsQueue().Get();
        init_info.NumFramesInFlight = s_FrameCount;
        init_info.RTVFormat = m_Swapchain->GetImageFormat();
        init_info.DSVFormat = m_Swapchain->GetDepthFormat();
        
        uint32_t imguiIndex = DirectX12DescriptorManager::Get()->AllocateSRVIndex();
        init_info.SrvDescriptorHeap = DirectX12DescriptorManager::Get()->GetSRVHeap();
        init_info.LegacySingleSrvCpuDescriptor = DirectX12DescriptorManager::Get()->GetSRVCPUHandle(imguiIndex);
        init_info.LegacySingleSrvGpuDescriptor = DirectX12DescriptorManager::Get()->GetSRVGPUHandle(imguiIndex);
        
        bool imguiInit = ImGui_ImplDX12_Init(&init_info);
        if (!imguiInit) {
            VECTOR_LOG_ERROR("ImGui_ImplDX12_Init FAILED!");
        } else {
            VECTOR_LOG_INFO("ImGui_ImplDX12_Init SUCCEEDED!");
        }

        io.Fonts->AddFontDefault();
        m_GameFont = io.Fonts->AddFontFromFileTTF("assets/font.ttf", 48.0f);

        bool imguiFont = ImGui_ImplDX12_CreateDeviceObjects();
        if (!imguiFont) {
            VECTOR_LOG_ERROR("ImGui_ImplDX12_CreateDeviceObjects FAILED!");
        } else {
            VECTOR_LOG_INFO("ImGui_ImplDX12_CreateDeviceObjects SUCCEEDED!");
        }
    }

    void DirectX12Renderer::BeginImGuiFrame() {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void DirectX12Renderer::EndImGuiFrame() {
        ImGui::Render();
        ID3D12DescriptorHeap* heaps[] = { DirectX12DescriptorManager::Get()->GetSRVHeap() };
        m_CommandList->SetDescriptorHeaps(1, heaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());
    }

    std::shared_ptr<Texture2D> DirectX12Renderer::AllocateTransientTexture(uint32_t handle, uint32_t width, uint32_t height, TextureFormat format, Texture2D* aliasTexture) {
        if (aliasTexture) {
            // Memory Aliasing: Create a texture that aliases the same allocation as aliasTexture
            auto dx12Tex = dynamic_cast<DirectX12Texture2D*>(aliasTexture);
            if (dx12Tex && dx12Tex->GetAllocation()) {
                return Texture2D::CreateRenderTargetAliased(width, height, format, dx12Tex);
            }
        }
        return Texture2D::CreateRenderTarget(width, height, format);
    }

    D3D12_RESOURCE_STATES MapRGStateToD3D12(int state) {
        switch (static_cast<RGResourceState>(state)) {
            case RGResourceState::RENDER_TARGET: return D3D12_RESOURCE_STATE_RENDER_TARGET;
            case RGResourceState::DEPTH_WRITE: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
            case RGResourceState::DEPTH_READ: return D3D12_RESOURCE_STATE_DEPTH_READ;
            case RGResourceState::SHADER_RESOURCE: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            case RGResourceState::UNORDERED_ACCESS: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            case RGResourceState::PRESENT: return D3D12_RESOURCE_STATE_PRESENT;
            default: return D3D12_RESOURCE_STATE_COMMON;
        }
    }

    void DirectX12Renderer::TransitionResources(const std::vector<RGResourceTransition>& transitions) {
        if (!m_FrameStarted || transitions.empty()) return;

        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(transitions.size());

        for (const auto& trans : transitions) {
            auto tex = m_RenderGraph.GetTexture(trans.handle);
            if (!tex) continue;

            auto dx12Tex = dynamic_cast<DirectX12Texture2D*>(tex.get());
            if (!dx12Tex || !dx12Tex->GetResource()) continue;

            D3D12_RESOURCE_STATES stateBefore = MapRGStateToD3D12(trans.oldState);
            D3D12_RESOURCE_STATES stateAfter = MapRGStateToD3D12(trans.newState);
            
            if (stateBefore == stateAfter) continue;

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = dx12Tex->GetResource();
            barrier.Transition.StateBefore = stateBefore;
            barrier.Transition.StateAfter = stateAfter;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            
            barriers.push_back(barrier);
        }

        if (!barriers.empty()) {
            m_CommandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }
    }

    void DirectX12Renderer::BeginUI() {
        // UI rendering handled by ImGui
    }

    void DirectX12Renderer::DrawUIRect(int x, int y, int w, int h, const glm::vec4& color) {
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2((float)x, (float)y),
            ImVec2((float)(x + w), (float)(y + h)),
            ImColor(color.r, color.g, color.b, color.a)
        );
    }

    void DirectX12Renderer::DrawUIText(const std::string& text, int x, int y, const glm::vec4& color, int fontSize) {
        ImGui::GetBackgroundDrawList()->AddText(
            m_GameFont ? m_GameFont : ImGui::GetFont(),
            (float)fontSize,
            ImVec2((float)x, (float)y),
            ImColor(color.r, color.g, color.b, color.a),
            text.c_str()
        );
    }

    void DirectX12Renderer::EndUI() {
        // UI rendering handled by ImGui
    }

    static float Halton(int index, int base) {
        float f = 1.0f;
        float r = 0.0f;
        while (index > 0) {
            f = f / (float)base;
            r = r + f * (float)(index % base);
            index = index / base;
        }
        return r;
    }

    void DirectX12Renderer::SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) {
        if (!m_FrameStarted) return; 

        struct PerFrameData {
            glm::mat4 view;
            glm::mat4 projection;
            glm::mat4 previousView;
            glm::mat4 previousProjection;
            glm::mat4 lightSpaceMatrix;
            glm::vec4 viewPos;
            glm::vec4 sunDir;
            glm::vec4 sunColor;
            glm::vec4 lightPos;
            glm::vec4 lightColor;
            int shadowMapIndex;
            int ssaoTexIndex;
            glm::vec2 jitter;
            glm::vec2 previousJitter;
        } pfd;

        glm::vec3 sunDir = glm::normalize(glm::vec3(-0.2f, 1.0f, 0.3f));
        glm::vec3 lightPos = sunDir * 80.0f;
        glm::mat4 lightProjection = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 1.0f, 150.0f);
        
        // Convert from OpenGL depth range [-1, 1] to DX12 depth range [0, 1]
        glm::mat4 dx12Projection = projection;
        dx12Projection[2][2] = 0.5f * projection[2][2] + 0.5f * projection[2][3];
        dx12Projection[3][2] = 0.5f * projection[3][2] + 0.5f * projection[3][3];
        
        m_UnjitteredProjection = dx12Projection;

        m_PreviousJitter = m_Jitter;
        if (m_TAAEnabled) {
            m_TAAFrameIndex++;
            int jitterPhaseCount = 8;
            int jitterIndex = m_TAAFrameIndex % jitterPhaseCount;
            float jitterX = (Halton(jitterIndex + 1, 2) - 0.5f) / (float)m_Swapchain->GetWidth();
            float jitterY = (Halton(jitterIndex + 1, 3) - 0.5f) / (float)m_Swapchain->GetHeight();
            m_Jitter = glm::vec2(jitterX, jitterY);
            
            dx12Projection[2][0] += m_Jitter.x * 2.0f;
            dx12Projection[2][1] += m_Jitter.y * 2.0f;
        } else {
            m_Jitter = glm::vec2(0.0f);
        }
        
        glm::mat4 dx12LightProj = lightProjection;
        dx12LightProj[2][2] = 0.5f * lightProjection[2][2] + 0.5f * lightProjection[2][3];
        dx12LightProj[3][2] = 0.5f * lightProjection[3][2] + 0.5f * lightProjection[3][3];

        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        m_LightSpaceMatrix = dx12LightProj * lightView;

        m_CurrentView = view;
        m_CurrentProjection = dx12Projection;

        pfd.view = view;
        pfd.projection = dx12Projection;
        pfd.previousView = m_PreviousView;
        pfd.previousProjection = m_PreviousProjection;
        pfd.jitter = m_Jitter;
        pfd.previousJitter = m_PreviousJitter;
        
        m_PreviousView = view;
        m_PreviousProjection = dx12Projection;

        pfd.lightSpaceMatrix = m_LightSpaceMatrix;
        pfd.viewPos = glm::vec4(viewPos, 1.0f);
        pfd.sunDir = glm::vec4(sunDir, 0.0f);
        pfd.sunColor = glm::vec4(1.0f);
        pfd.lightPos = glm::vec4(0.0f);
        pfd.lightColor = glm::vec4(1.0f);
        pfd.shadowMapIndex = m_ShadowPass ? m_ShadowPass->GetSRVIndex() : -1;
        pfd.ssaoTexIndex = (m_SSAO && m_SSAOEnabled) ? m_SSAO->GetSSAOSRVIndex() : -1;

        m_PerFrameUBOs[m_FrameIndex]->SetData(&pfd, sizeof(PerFrameData), 0);
    }

    void DirectX12Renderer::BindObjectAndMaterial(const RenderCommand& cmd, bool bindMaterial) {
        if (m_ObjectDataIndex >= m_ObjectDataPool.size()) {
            ObjectData data;
            data.ubo = std::make_unique<DirectX12UniformBuffer>(static_cast<uint32_t>(sizeof(glm::mat4) * 101), 2);
            m_ObjectDataPool.push_back(std::move(data));
        }
        
        struct PerObjectData {
            glm::mat4 model;
            glm::mat4 bones[100];
        } data;
        
        data.model = cmd.model;
        
        if (cmd.boneTransforms && !cmd.boneTransforms->empty()) {
            size_t count = (std::min)((size_t)100, cmd.boneTransforms->size());
            for (size_t i = 0; i < count; ++i) {
                data.bones[i] = (*cmd.boneTransforms)[i];
            }
        } else {
            for (size_t i = 0; i < 100; ++i) {
                data.bones[i] = glm::mat4(1.0f);
            }
        }
        
        m_ObjectDataPool[m_ObjectDataIndex].ubo->SetData(&data, sizeof(PerObjectData));
        D3D12_GPU_VIRTUAL_ADDRESS objectDataAddress = static_cast<DirectX12UniformBuffer*>(m_ObjectDataPool[m_ObjectDataIndex].ubo.get())->GetGPUVirtualAddress();
        m_CommandList->SetGraphicsRootConstantBufferView(2, objectDataAddress);
        m_ObjectDataIndex++;

        if (bindMaterial && cmd.material) {
            if (m_MaterialDataIndex >= m_MaterialDataPool.size()) {
                MaterialDataBlock matBlock;
                matBlock.ubo = std::make_unique<DirectX12UniformBuffer>(static_cast<uint32_t>(sizeof(float) * 16), 2);
                m_MaterialDataPool.push_back(std::move(matBlock));
            }

            struct MaterialData {
                glm::vec4 albedoColor;
                int hasAlbedoMap;
                int hasNormalMap;
                int hasMetallicRoughnessMap;
                int hasAOMap;
                float metallicFactor;
                float roughnessFactor;
                int isUnlit;
                int padding;
                int albedoMapIndex;
                int normalMapIndex;
                int metallicRoughnessMapIndex;
                int aoMapIndex;
            } matData;
            
            matData.albedoColor = cmd.material->albedoColor;
            matData.metallicFactor = cmd.material->metallic;
            matData.roughnessFactor = cmd.material->roughness;
            matData.isUnlit = cmd.material->isUnlit ? 1 : 0;
            matData.padding = 0;
            
            matData.hasAlbedoMap = cmd.material->albedoTexture ? 1 : 0;
            matData.albedoMapIndex = cmd.material->albedoTexture ? static_cast<DirectX12Texture2D*>(cmd.material->albedoTexture.get())->GetDescriptorIndex() : -1;

            matData.hasNormalMap = cmd.material->normalTexture ? 1 : 0;
            matData.normalMapIndex = cmd.material->normalTexture ? static_cast<DirectX12Texture2D*>(cmd.material->normalTexture.get())->GetDescriptorIndex() : -1;

            matData.hasMetallicRoughnessMap = cmd.material->metallicRoughnessTexture ? 1 : 0;
            matData.metallicRoughnessMapIndex = cmd.material->metallicRoughnessTexture ? static_cast<DirectX12Texture2D*>(cmd.material->metallicRoughnessTexture.get())->GetDescriptorIndex() : -1;

            matData.hasAOMap = cmd.material->aoTexture ? 1 : 0;
            matData.aoMapIndex = cmd.material->aoTexture ? static_cast<DirectX12Texture2D*>(cmd.material->aoTexture.get())->GetDescriptorIndex() : -1;

            m_MaterialDataPool[m_MaterialDataIndex].ubo->SetData(&matData, sizeof(MaterialData));
            D3D12_GPU_VIRTUAL_ADDRESS materialDataAddress = static_cast<DirectX12UniformBuffer*>(m_MaterialDataPool[m_MaterialDataIndex].ubo.get())->GetGPUVirtualAddress();
            m_CommandList->SetGraphicsRootConstantBufferView(3, materialDataAddress);
            m_MaterialDataIndex++;
        }
    }

    void DirectX12Renderer::SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) {
        if (m_LightData.numPointLights < 64) {
            int idx = m_LightData.numPointLights++;
            m_LightData.pointLights[idx * 2 + 0] = glm::vec4(position, radius);
            m_LightData.pointLights[idx * 2 + 1] = glm::vec4(color, intensity);
        }
    }

    void DirectX12Renderer::SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) {
        m_LightData.dirLightDir = glm::vec4(glm::normalize(direction), 0.0f);
        m_LightData.dirLightColor = glm::vec4(color, intensity);
    }

    void DirectX12Renderer::BeginMainPass(std::shared_ptr<Texture2D> inNormal, std::shared_ptr<Texture2D> inPosition, std::shared_ptr<Texture2D> inDepth, std::shared_ptr<Texture2D> outColor) {
        if (!m_FrameStarted) return;
        m_LightUBOs[m_FrameIndex]->SetData(&m_LightData, sizeof(LightUBOData));
        
        auto dx12Color = std::dynamic_pointer_cast<DirectX12Texture2D>(outColor);
        if (!dx12Color) return;

        if (!m_MainRTVHeap) {
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = 1;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            m_Context->GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_MainRTVHeap));
            m_Context->GetDevice()->CreateRenderTargetView(dx12Color->GetResource(), nullptr, m_MainRTVHeap->GetCPUDescriptorHandleForHeapStart());
        }

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_Swapchain->GetDepthBufferDSV();
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_MainRTVHeap->GetCPUDescriptorHandleForHeapStart();

        m_PostProcessor->BeginMainPass(m_CommandList.Get(), rtvHandle, dsvHandle);

        m_CommandList->SetGraphicsRootSignature(m_Pipeline->GetRootSignature());
        m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = (float)m_Swapchain->GetWidth();
        viewport.Height = (float)m_Swapchain->GetHeight();
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_CommandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect = {};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = m_Swapchain->GetWidth();
        scissorRect.bottom = m_Swapchain->GetHeight();
        m_CommandList->RSSetScissorRects(1, &scissorRect);

        // Bind b0 (PerFrame) and b1 (Light)
        auto pfdAddress = static_cast<DirectX12UniformBuffer*>(m_PerFrameUBOs[m_FrameIndex].get())->GetGPUVirtualAddress();
        auto lightAddress = static_cast<DirectX12UniformBuffer*>(m_LightUBOs[m_FrameIndex].get())->GetGPUVirtualAddress();
        
        m_CommandList->SetGraphicsRootConstantBufferView(0, pfdAddress);
        m_CommandList->SetGraphicsRootConstantBufferView(1, lightAddress);
    }

    void DirectX12Renderer::BeginPrepass(std::shared_ptr<Texture2D> outNormal, std::shared_ptr<Texture2D> outPosition, std::shared_ptr<Texture2D> outDepth) {
        if (!m_FrameStarted) return;
        m_Prepass->BeginPass(m_CommandList.Get(), outNormal, outPosition, outDepth);
    }

    void DirectX12Renderer::FlushPrepass() {
        if (!m_FrameStarted) return;
        
        for (const auto& cmd : m_RenderQueue) {
            BindObjectAndMaterial(cmd, false); // Prepass only needs object data, materials could be bound if alpha testing but we don't have it yet

            auto pfdAddress = static_cast<DirectX12UniformBuffer*>(m_PerFrameUBOs[m_FrameIndex].get())->GetGPUVirtualAddress();
            m_CommandList->SetGraphicsRootConstantBufferView(0, pfdAddress);

            if (cmd.mesh) {
                const DirectX12Mesh* dx12Mesh = dynamic_cast<const DirectX12Mesh*>(cmd.mesh);
                if (dx12Mesh) {
                    dx12Mesh->Bind(m_CommandList.Get());
                    m_CommandList->DrawIndexedInstanced(dx12Mesh->GetIndexCount(), 1, 0, 0, 0);
                }
            }
        }
        
        m_Prepass->EndPass(m_CommandList.Get());
    }

    void DirectX12Renderer::GenerateSSAO() {
        if (!m_FrameStarted) return;
        if (m_SSAOEnabled) {
            m_SSAO->Generate(m_CommandList.Get(), 
                             m_Prepass->GetNormalSRVIndex(), 
                             m_Prepass->GetDepthSRVIndex(), 
                             m_UnjitteredProjection,
                             m_CurrentView);
        }
    }

    void DirectX12Renderer::BeginShadowPass() {
        if (!m_FrameStarted) return;
        m_ShadowPass->BeginPass(m_CommandList.Get());
    }

    void DirectX12Renderer::FlushShadowPass() {
        if (!m_FrameStarted) return;
        
        for (const auto& cmd : m_RenderQueue) {
            BindObjectAndMaterial(cmd, false); // Shadow pass doesn't need material data

            // Bind b0 for shadow pass (PerFrameData with light space matrix)
            auto pfdAddress = static_cast<DirectX12UniformBuffer*>(m_PerFrameUBOs[m_FrameIndex].get())->GetGPUVirtualAddress();
            m_CommandList->SetGraphicsRootConstantBufferView(0, pfdAddress);

            if (cmd.mesh) {
                const DirectX12Mesh* dx12Mesh = dynamic_cast<const DirectX12Mesh*>(cmd.mesh);
                if (dx12Mesh) {
                    dx12Mesh->Bind(m_CommandList.Get());
                    m_CommandList->DrawIndexedInstanced(dx12Mesh->GetIndexCount(), 1, 0, 0, 0);
                }
            }
        }
        
        m_ShadowPass->EndPass(m_CommandList.Get());
    }

    void DirectX12Renderer::FlushMainPass() {
        if (!m_FrameStarted) return;
        
        if (m_WireframeMode && m_WireframePipeline) {
            m_CommandList->SetPipelineState(m_WireframePipeline->GetPipelineState());
        } else {
            m_CommandList->SetPipelineState(m_Pipeline->GetPipelineState());
        }

        for (const auto& cmd : m_RenderQueue) {
            BindObjectAndMaterial(cmd, true); // Main pass needs object and material data
            
            if (cmd.mesh) {
                const DirectX12Mesh* dx12Mesh = dynamic_cast<const DirectX12Mesh*>(cmd.mesh);
                if (dx12Mesh) {
                    dx12Mesh->Bind(m_CommandList.Get());
                    m_CommandList->DrawIndexedInstanced(dx12Mesh->GetIndexCount(), 1, 0, 0, 0);
                }
            }
        }

        // Draw Skybox
        if (m_CurrentSkybox && m_SkyboxPipeline) {
            m_CommandList->SetPipelineState(m_SkyboxPipeline->GetPipelineState());
            
            if (m_MaterialDataIndex >= m_MaterialDataPool.size()) {
                MaterialDataBlock data;
                data.ubo = std::make_unique<DirectX12UniformBuffer>(static_cast<uint32_t>(sizeof(float) * 16), 2);
                m_MaterialDataPool.push_back(std::move(data));
            }

            struct MaterialData {
                glm::vec4 albedoColor;
                int hasAlbedoMap;
                int hasNormalMap;
                int hasMetallicRoughnessMap;
                int hasAOMap;
                float metallicFactor;
                float roughnessFactor;
                int isUnlit;
                int padding;
                int albedoMapIndex;
                int normalMapIndex;
                int metallicRoughnessMapIndex;
                int aoMapIndex;
            } matData;
            
            memset(&matData, 0, sizeof(MaterialData));
            matData.albedoColor = glm::vec4(1.0f);
            matData.isUnlit = 1;
            matData.hasAlbedoMap = 1;
            matData.albedoMapIndex = m_CurrentSkybox->GetDescriptorIndex();

            m_MaterialDataPool[m_MaterialDataIndex].ubo->SetData(&matData, sizeof(MaterialData), 0);
            D3D12_GPU_VIRTUAL_ADDRESS skyboxMatAddress = static_cast<DirectX12UniformBuffer*>(m_MaterialDataPool[m_MaterialDataIndex].ubo.get())->GetGPUVirtualAddress();
            m_MaterialDataIndex++;

            m_CommandList->SetGraphicsRootConstantBufferView(3, skyboxMatAddress);
            m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_CommandList->DrawInstanced(36, 1, 0, 0);
        }

        m_RenderQueue.clear();
        m_ObjectDataIndex = 0;
        m_MaterialDataIndex = 0;
        m_LightData.numPointLights = 0;
        m_CurrentSkybox = nullptr;
    }

    void DirectX12Renderer::SubmitSkybox(Cubemap* cubemap) {
        m_CurrentSkybox = dynamic_cast<DirectX12Cubemap*>(cubemap);
    }

    void DirectX12Renderer::EndPostProcessPass(std::shared_ptr<Texture2D> inColor) {
        if (!m_FrameStarted) return;
        
        Microsoft::WRL::ComPtr<IDXGISwapChain3> swapchain3;
        uint32_t backBufferIndex = 0;
        if (SUCCEEDED(m_Swapchain->GetSwapchain().As(&swapchain3))) {
            backBufferIndex = swapchain3->GetCurrentBackBufferIndex();
        }
        
        auto dx12Color = std::dynamic_pointer_cast<DirectX12Texture2D>(inColor);
        if (!dx12Color) return;
        
        uint32_t postProcessInputSRV = dx12Color->GetDescriptorIndex();

        if (m_TAAEnabled) {
            // TAA Resolve passes:
            // 1. Current frame HDR image (from PostProcessor's HDR buffer before bloom/tonemap)
            // 2. Motion vectors (from Prepass)
            // 3. Depth buffer (from Prepass)
            m_TAA->Resolve(
                m_CommandList.Get(), 
                postProcessInputSRV, 
                m_Prepass->GetMotionVectorSRVIndex(), 
                m_Prepass->GetDepthSRVIndex()
            );

            // Output of TAA is now the input for Bloom / Tonemapping
            postProcessInputSRV = m_TAA->GetResolvedSRVIndex();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_Swapchain->GetBackBufferRTV(backBufferIndex);
        
        m_PostProcessor->Resolve(m_CommandList.Get(), rtvHandle, m_Swapchain->GetWidth(), m_Swapchain->GetHeight(), postProcessInputSRV);
    }

    void DirectX12Renderer::SetWireframeMode(bool enabled) {
        if (m_WireframeMode != enabled) {
            WaitIdle(); // Ensure GPU is done before changing state
            m_WireframeMode = enabled;
        }
    }

    void DirectX12Renderer::SetBloomEnabled(bool enabled) {
        if (m_PostProcessor) {
            m_PostProcessor->m_BloomEnabled = enabled;
        }
    }

    bool DirectX12Renderer::IsBloomEnabled() const {
        if (m_PostProcessor) {
            return m_PostProcessor->m_BloomEnabled;
        }
        return false;
    }

} // namespace VECTOR
