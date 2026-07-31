#include "Engine/Graphics/DirectX12/DirectX12SSAO.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Pipeline.hpp"
#include "Engine/Graphics/DirectX12/DirectX12UniformBuffer.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Shader.hpp"
#include "Engine/Graphics/DirectX12/DirectX12DescriptorManager.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <directx/d3dx12.h>
#include <random>

namespace VECTOR {

    static float lerp(float a, float b, float f) {
        return a + f * (b - a);
    }

    DirectX12SSAO::DirectX12SSAO(DirectX12Context* context, uint32_t width, uint32_t height)
        : m_Context(context), m_Width(width), m_Height(height) {
    }

    DirectX12SSAO::~DirectX12SSAO() {
        m_SSAOTexture.Reset();
        m_BlurTexture.Reset();
        m_NoiseTexture.Reset();
        m_NoiseUploadBuffer.Reset();
        m_RTVHeap.Reset();
    }

    void DirectX12SSAO::Initialize() {
        GenerateKernel();
        CreateResources();
        CreateNoiseTexture();
        CreateDescriptors();
        CreatePipelines();

        m_SSAODataBuffer = std::make_unique<DirectX12UniformBuffer>(sizeof(SSAODataBlock), 1);
    }

    void DirectX12SSAO::Resize(uint32_t width, uint32_t height) {
        if (m_Width == width && m_Height == height) return;
        m_Width = width;
        m_Height = height;

        m_SSAOTexture.Reset();
        m_BlurTexture.Reset();
        
        CreateResources();
        CreateDescriptors();
    }

    void DirectX12SSAO::GenerateKernel() {
        std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
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

            m_SSAOData.samples[i] = glm::vec4(sample, 0.0f);
        }
    }

    void DirectX12SSAO::CreateResources() {
        auto device = m_Context->GetDevice();

        D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, m_Width, m_Height, 1, 1);
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_R8_UNORM;
        clearValue.Color[0] = 1.0f;
        clearValue.Color[1] = 1.0f;
        clearValue.Color[2] = 1.0f;
        clearValue.Color[3] = 1.0f;

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        
        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &clearValue,
            IID_PPV_ARGS(&m_SSAOTexture)
        );

        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &clearValue,
            IID_PPV_ARGS(&m_BlurTexture)
        );
    }

    void DirectX12SSAO::CreateNoiseTexture() {
        auto device = m_Context->GetDevice();

        std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
        std::default_random_engine generator;

        std::vector<glm::vec4> ssaoNoise;
        for (int i = 0; i < 16; i++) {
            glm::vec4 noise(
                randomFloats(generator) * 2.0f - 1.0f,
                randomFloats(generator) * 2.0f - 1.0f,
                0.0f,
                1.0f
            );
            ssaoNoise.push_back(noise);
        }

        D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, 4, 4, 1, 1);
        
        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_NoiseTexture)
        );

        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_NoiseTexture.Get(), 0, 1);
        auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
        
        device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_NoiseUploadBuffer)
        );

        D3D12_SUBRESOURCE_DATA noiseData = {};
        noiseData.pData = ssaoNoise.data();
        noiseData.RowPitch = 4 * sizeof(glm::vec4);
        noiseData.SlicePitch = noiseData.RowPitch * 4;

        // Need a command list to copy the data. We'll assume this happens before rendering.
        // For simplicity in engine init, we will just use a temporary command list here.
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
        device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList));

        UpdateSubresources(cmdList.Get(), m_NoiseTexture.Get(), m_NoiseUploadBuffer.Get(), 0, 0, 1, &noiseData);

        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_NoiseTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrier);

        cmdList->Close();
        ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
        m_Context->GetGraphicsQueue()->ExecuteCommandLists(1, ppCommandLists);

        // Wait for generation
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        m_Context->GetGraphicsQueue()->Signal(fence.Get(), 1);

        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        fence->SetEventOnCompletion(1, eventHandle);
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    void DirectX12SSAO::CreateDescriptors() {
        auto device = m_Context->GetDevice();
        auto descManager = DirectX12DescriptorManager::Get();

        // RTV Heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 2; // SSAO, Blur
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap));

        auto rtvHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_SSAORTV = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
        device->CreateRenderTargetView(m_SSAOTexture.Get(), nullptr, m_SSAORTV);

        m_BlurRTV = m_SSAORTV;
        m_BlurRTV.ptr += rtvHandleSize;
        device->CreateRenderTargetView(m_BlurTexture.Get(), nullptr, m_BlurRTV);

        // SRV indices
        if (m_SSAOSRVIndex == 0) m_SSAOSRVIndex = descManager->AllocateSRVIndex();
        if (m_BlurSRVIndex == 0) m_BlurSRVIndex = descManager->AllocateSRVIndex();
        if (m_NoiseSRVIndex == 0) m_NoiseSRVIndex = descManager->AllocateSRVIndex();

        device->CreateShaderResourceView(m_SSAOTexture.Get(), nullptr, descManager->GetSRVCPUHandle(m_SSAOSRVIndex));
        device->CreateShaderResourceView(m_BlurTexture.Get(), nullptr, descManager->GetSRVCPUHandle(m_BlurSRVIndex));
        device->CreateShaderResourceView(m_NoiseTexture.Get(), nullptr, descManager->GetSRVCPUHandle(m_NoiseSRVIndex));
    }

    void DirectX12SSAO::CreatePipelines() {
        DirectX12PipelineConfig config;
        config.numRenderTargets = 1;
        config.rtvFormats[0] = DXGI_FORMAT_R8_UNORM;
        config.isDepthOnly = false;
        config.dsvFormat = DXGI_FORMAT_UNKNOWN; // No depth buffer for fullscreen pass

        auto ssaoShader = ResourceManager::Get().LoadShader("SSAO", "assets/engine/shaders/dx12/fullscreen.hlsl", "assets/engine/shaders/dx12/ssao.hlsl");
        m_SSAOPipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(ssaoShader.get()), config);

        auto blurShader = ResourceManager::Get().LoadShader("SSAOBlur", "assets/engine/shaders/dx12/fullscreen.hlsl", "assets/engine/shaders/dx12/ssao_blur.hlsl");
        m_BlurPipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(blurShader.get()), config);
    }

    void DirectX12SSAO::Generate(ID3D12GraphicsCommandList* commandList, 
                                 uint32_t normalSRVIndex, 
                                 uint32_t depthSRVIndex,
                                 const glm::mat4& projection, 
                                 const glm::mat4& view) 
    {
        m_SSAOData.projection = projection;
        m_SSAOData.invProjection = glm::inverse(projection);
        m_SSAOData.screenSize = glm::vec2(m_Width, m_Height);
        m_SSAOData.normalTexIndex = normalSRVIndex;
        m_SSAOData.depthTexIndex = depthSRVIndex;
        m_SSAOData.noiseTexIndex = m_NoiseSRVIndex;
        m_SSAOData.ssaoTexIndex = m_SSAOSRVIndex;
        m_SSAODataBuffer->SetData(&m_SSAOData, sizeof(SSAODataBlock), 0);

        // 1. Generate SSAO
        D3D12_RESOURCE_BARRIER ssaoBarriers[2] = {};
        ssaoBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_SSAOTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, ssaoBarriers);

        commandList->OMSetRenderTargets(1, &m_SSAORTV, FALSE, nullptr);
        
        commandList->SetGraphicsRootSignature(m_SSAOPipeline->GetRootSignature());
        commandList->SetPipelineState(m_SSAOPipeline->GetPipelineState());
        commandList->SetGraphicsRootConstantBufferView(0, m_SSAODataBuffer->GetGPUVirtualAddress());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, 1.0f };
        commandList->RSSetViewports(1, &viewport);
        D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height) };
        commandList->RSSetScissorRects(1, &scissorRect);
        
        commandList->DrawInstanced(3, 1, 0, 0);

        // 2. Blur SSAO
        D3D12_RESOURCE_BARRIER blurBarriers[2] = {};
        blurBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_SSAOTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        blurBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_BlurTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(2, blurBarriers);

        commandList->OMSetRenderTargets(1, &m_BlurRTV, FALSE, nullptr);

        commandList->SetGraphicsRootSignature(m_BlurPipeline->GetRootSignature());
        commandList->SetPipelineState(m_BlurPipeline->GetPipelineState());
        commandList->SetGraphicsRootConstantBufferView(0, m_SSAODataBuffer->GetGPUVirtualAddress());

        commandList->DrawInstanced(3, 1, 0, 0);

        // Finish
        D3D12_RESOURCE_BARRIER finalBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_BlurTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &finalBarrier);
    }

} // namespace VECTOR
