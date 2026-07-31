#define NOMINMAX
#include "Engine/Graphics/DirectX12/DirectX12Renderer.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Mesh.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Texture2D.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Cubemap.hpp"
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

        m_Swapchain = std::make_unique<DirectX12Swapchain>(m_Context.get(), m_Window, width, height);

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
            m_LightUBOs.push_back(std::make_unique<DirectX12UniformBuffer>(sizeof(LightUBOData), 1));
        }
        // Dummy Object Data
        m_DummyObjectUBO = std::make_unique<DirectX12UniformBuffer>(sizeof(glm::mat4) * 101, 2);

        // m_MaterialDataPool is used for materials

        // Skybox pipeline will be created in SubmitSkybox if needed, or initialized here
        // (will initialize later if needed)
        VECTOR_LOG_INFO("UBOs created.");
        

        // Create Pipeline
        VECTOR_LOG_INFO("Creating DirectX 12 Pipeline...");
        auto shader = ResourceManager::Get().LoadShader("Default3D", "assets/engine/shaders/dx12/main3D.hlsl", "assets/engine/shaders/dx12/main3D.hlsl");
        ResourceManager::Get().LoadShader("Main3D", "assets/engine/shaders/dx12/main3D.hlsl", "assets/engine/shaders/dx12/main3D.hlsl");
        m_Pipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(shader.get()), D3D12_COMPARISON_FUNC_LESS_EQUAL);
        
        VECTOR_LOG_INFO("Creating DirectX 12 Wireframe Pipeline...");
        m_WireframePipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(shader.get()), D3D12_COMPARISON_FUNC_LESS_EQUAL, false, true);

        VECTOR_LOG_INFO("Creating DirectX 12 Skybox Pipeline...");
        auto skyboxShader = ResourceManager::Get().LoadShader("Skybox", "assets/engine/shaders/dx12/skybox.hlsl", "assets/engine/shaders/dx12/skybox.hlsl");
        m_SkyboxPipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(skyboxShader.get()), D3D12_COMPARISON_FUNC_LESS_EQUAL);

        VECTOR_LOG_INFO("Initializing DirectX 12 Shadow Pass...");
        m_ShadowPass = std::make_unique<DirectX12ShadowPass>(m_Context.get());
        m_ShadowPass->Initialize();

        m_Prepass = std::make_unique<DirectX12Prepass>(m_Context.get(), width, height);
        m_Prepass->Initialize();

        m_SSAO = std::make_unique<DirectX12SSAO>(m_Context.get(), width, height);
        m_SSAO->Initialize();

        VECTOR_LOG_INFO("DirectX 12 Renderer Initialized.");
        return true;
    }

    void DirectX12Renderer::Shutdown() {
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
        m_SkyboxPipeline.reset();
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
            
            ID3D12DescriptorHeap* descriptorHeaps[] = { DirectX12DescriptorManager::Get()->GetSRVHeap() };
            m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
            
            m_FrameStarted = true;
        }

        float clearColor[] = { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
        m_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
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
        WaitIdle();
        m_Swapchain->Recreate(width, height);
    }

    void DirectX12Renderer::SetFullscreen(bool fullscreen, bool borderless) {
        SDL_SetWindowFullscreen(m_Window, fullscreen);
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

    void DirectX12Renderer::SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) {
        if (!m_FrameStarted) return; 

        struct PerFrameData {
            glm::mat4 view;
            glm::mat4 projection;
            glm::mat4 lightSpaceMatrix;
            glm::vec4 viewPos;
            glm::vec4 sunDir;
            glm::vec4 sunColor;
            glm::vec4 lightPos;
            glm::vec4 lightColor;
            int shadowMapIndex;
            int ssaoTexIndex;
            int padding[2];
        } pfd;

        glm::vec3 sunDir = glm::normalize(glm::vec3(-0.2f, 1.0f, 0.3f));
        glm::vec3 lightPos = sunDir * 80.0f;
        glm::mat4 lightProjection = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 1.0f, 150.0f);
        
        // Convert from OpenGL depth range [-1, 1] to DX12 depth range [0, 1]
        glm::mat4 dx12Projection = projection;
        dx12Projection[2][2] = 0.5f * projection[2][2] + 0.5f * projection[2][3];
        dx12Projection[3][2] = 0.5f * projection[3][2] + 0.5f * projection[3][3];
        
        glm::mat4 dx12LightProj = lightProjection;
        dx12LightProj[2][2] = 0.5f * lightProjection[2][2] + 0.5f * lightProjection[2][3];
        dx12LightProj[3][2] = 0.5f * lightProjection[3][2] + 0.5f * lightProjection[3][3];

        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        m_LightSpaceMatrix = dx12LightProj * lightView;

        m_CurrentView = view;
        m_CurrentProjection = dx12Projection;

        pfd.view = view;
        pfd.projection = dx12Projection;
        pfd.lightSpaceMatrix = m_LightSpaceMatrix;
        pfd.viewPos = glm::vec4(viewPos, 1.0f);
        pfd.sunDir = glm::vec4(sunDir, 0.0f);
        pfd.sunColor = glm::vec4(1.0f);
        pfd.lightPos = glm::vec4(0.0f);
        pfd.lightColor = glm::vec4(1.0f);
        pfd.shadowMapIndex = m_ShadowPass ? m_ShadowPass->GetSRVIndex() : -1;
        pfd.ssaoTexIndex = m_SSAO ? m_SSAO->GetSSAOSRVIndex() : -1;

        m_PerFrameUBOs[m_FrameIndex]->SetData(&pfd, sizeof(PerFrameData), 0);
    }

    void DirectX12Renderer::SubmitMesh(const Mesh* mesh, const Material* material, const glm::mat4& model, const std::vector<glm::mat4>* boneTransforms) {
        RenderCommand cmd;
        cmd.mesh = mesh;
        cmd.material = material;
        cmd.model = model;
        cmd.boneTransforms = boneTransforms;

        if (m_ObjectDataIndex >= m_ObjectDataPool.size()) {
            ObjectData data;
            data.ubo = std::make_unique<DirectX12UniformBuffer>(sizeof(glm::mat4) * 101, 2);
            m_ObjectDataPool.push_back(std::move(data));
        }
        
        struct PerObjectData {
            glm::mat4 model;
            glm::mat4 bones[100];
        } data;
        
        data.model = model;
        
        if (boneTransforms && !boneTransforms->empty()) {
            size_t count = (std::min)((size_t)100, boneTransforms->size());
            for (size_t i = 0; i < count; ++i) {
                data.bones[i] = (*boneTransforms)[i];
            }
        } else {
            for (size_t i = 0; i < 100; ++i) {
                data.bones[i] = glm::mat4(1.0f);
            }
        }
        
        m_ObjectDataPool[m_ObjectDataIndex].ubo->SetData(&data, sizeof(PerObjectData));
        cmd.objectDataAddress = static_cast<DirectX12UniformBuffer*>(m_ObjectDataPool[m_ObjectDataIndex].ubo.get())->GetGPUVirtualAddress();
        m_ObjectDataIndex++;

        if (material) {
            if (m_MaterialDataIndex >= m_MaterialDataPool.size()) {
                MaterialDataBlock data;
                data.ubo = std::make_unique<DirectX12UniformBuffer>(sizeof(float) * 16, 2);
                m_MaterialDataPool.push_back(std::move(data));
            }

            struct MaterialData {
                glm::vec4 albedoColor;
                float specularStrength;
                float shininess;
                int isUnlit;
                int hasTexture;
                int diffuseTextureIndex;
                int padding[3];
            } matData;
            
            matData.albedoColor = material->albedoColor;
            matData.specularStrength = material->metallic;
            matData.shininess = material->roughness;
            matData.isUnlit = material->isUnlit ? 1 : 0;
            
            if (material->albedoTexture) {
                matData.hasTexture = 1;
                matData.diffuseTextureIndex = static_cast<DirectX12Texture2D*>(material->albedoTexture.get())->GetDescriptorIndex();
            } else {
                matData.hasTexture = 0;
                matData.diffuseTextureIndex = -1;
            }

            m_MaterialDataPool[m_MaterialDataIndex].ubo->SetData(&matData, sizeof(MaterialData));
            cmd.materialDataAddress = static_cast<DirectX12UniformBuffer*>(m_MaterialDataPool[m_MaterialDataIndex].ubo.get())->GetGPUVirtualAddress();
            m_MaterialDataIndex++;
        }

        m_RenderQueue.push_back(std::move(cmd));
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

    void DirectX12Renderer::BeginMainPass() {
        if (!m_FrameStarted) return;
        m_LightUBOs[m_FrameIndex]->SetData(&m_LightData, sizeof(LightUBOData));
        
        uint32_t backBufferIndex = m_Swapchain->AcquireNextImage();
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_Swapchain->GetBackBufferRTV(backBufferIndex);
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_Swapchain->GetDepthBufferDSV();
        m_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

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

    void DirectX12Renderer::BeginPrepass() {
        if (!m_FrameStarted) return;
        m_Prepass->BeginPass(m_CommandList.Get());
    }

    void DirectX12Renderer::FlushPrepass() {
        if (!m_FrameStarted) return;
        
        for (const auto& cmd : m_RenderQueue) {
            m_CommandList->SetGraphicsRootConstantBufferView(2, cmd.objectDataAddress);

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

        // Generate SSAO
        m_SSAO->Generate(m_CommandList.Get(), 
                         m_Prepass->GetNormalSRVIndex(), 
                         m_Prepass->GetDepthSRVIndex(), 
                         m_CurrentProjection,
                         m_CurrentView);
    }

    void DirectX12Renderer::BeginShadowPass() {
        if (!m_FrameStarted) return;
        m_ShadowPass->BeginPass(m_CommandList.Get());
    }

    void DirectX12Renderer::FlushShadowPass() {
        if (!m_FrameStarted) return;
        
        for (const auto& cmd : m_RenderQueue) {
            m_CommandList->SetGraphicsRootConstantBufferView(2, cmd.objectDataAddress);

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
            // Bind b2 (ObjectData)
            m_CommandList->SetGraphicsRootConstantBufferView(2, cmd.objectDataAddress);

            // Bind b3 (MaterialData)
            if (cmd.materialDataAddress != 0) {
                m_CommandList->SetGraphicsRootConstantBufferView(3, cmd.materialDataAddress);
            }
            
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
                data.ubo = std::make_unique<DirectX12UniformBuffer>(sizeof(float) * 16, 2);
                m_MaterialDataPool.push_back(std::move(data));
            }

            struct MaterialData {
                glm::vec4 albedoColor;
                float specularStrength;
                float shininess;
                int isUnlit;
                int hasTexture;
                int diffuseTextureIndex;
                int padding[3];
            } matData;
            
            matData.albedoColor = glm::vec4(1.0f);
            matData.isUnlit = 1;
            matData.hasTexture = 1;
            matData.diffuseTextureIndex = m_CurrentSkybox->GetDescriptorIndex();

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

} // namespace VECTOR
