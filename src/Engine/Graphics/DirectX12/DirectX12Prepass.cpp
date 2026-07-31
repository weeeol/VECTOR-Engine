#include "Engine/Graphics/DirectX12/DirectX12Prepass.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Pipeline.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Shader.hpp"
#include "Engine/Graphics/DirectX12/DirectX12DescriptorManager.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <directx/d3dx12.h>

namespace VECTOR {

    DirectX12Prepass::DirectX12Prepass(DirectX12Context* context, uint32_t width, uint32_t height)
        : m_Context(context), m_Width(width), m_Height(height) {
    }

    DirectX12Prepass::~DirectX12Prepass() {
        m_NormalTexture.Reset();
        m_DepthTexture.Reset();
        m_RTVHeap.Reset();
        m_DSVHeap.Reset();
    }

    void DirectX12Prepass::Initialize() {
        CreateResources(m_Width, m_Height);
        CreateDescriptors();
        CreatePipeline();
    }

    void DirectX12Prepass::Resize(uint32_t width, uint32_t height) {
        if (m_Width == width && m_Height == height) return;
        m_Width = width;
        m_Height = height;

        m_NormalTexture.Reset();
        m_DepthTexture.Reset();
        
        CreateResources(m_Width, m_Height);
        CreateDescriptors();
    }

    void DirectX12Prepass::CreateResources(uint32_t width, uint32_t height) {
        auto device = m_Context->GetDevice();

        // 1. Create Normal Render Target (R16G16B16A16_FLOAT)
        D3D12_RESOURCE_DESC rtvDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, 1, 1);
        rtvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE rtvClearValue = {};
        rtvClearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        rtvClearValue.Color[0] = 0.0f;
        rtvClearValue.Color[1] = 0.0f;
        rtvClearValue.Color[2] = 0.0f;
        rtvClearValue.Color[3] = 1.0f;

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &rtvDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &rtvClearValue,
            IID_PPV_ARGS(&m_NormalTexture)
        );

        // 2. Create Depth Render Target (D32_FLOAT)
        D3D12_RESOURCE_DESC dsvDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, width, height, 1, 1);
        dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE dsvClearValue = {};
        dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        dsvClearValue.DepthStencil.Depth = 1.0f;
        dsvClearValue.DepthStencil.Stencil = 0;

        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &dsvDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &dsvClearValue,
            IID_PPV_ARGS(&m_DepthTexture)
        );
    }

    void DirectX12Prepass::CreateDescriptors() {
        auto device = m_Context->GetDevice();
        auto descManager = DirectX12DescriptorManager::Get();

        // RTV Heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 1;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap));

        m_NormalRTV = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
        device->CreateRenderTargetView(m_NormalTexture.Get(), nullptr, m_NormalRTV);

        // DSV Heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap));

        m_DepthDSV = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvViewDesc = {};
        dsvViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvViewDesc.Texture2D.MipSlice = 0;
        device->CreateDepthStencilView(m_DepthTexture.Get(), &dsvViewDesc, m_DepthDSV);

        // SRV
        if (m_NormalSRVIndex == 0) m_NormalSRVIndex = descManager->AllocateSRVIndex();
        if (m_DepthSRVIndex == 0) m_DepthSRVIndex = descManager->AllocateSRVIndex();

        device->CreateShaderResourceView(m_NormalTexture.Get(), nullptr, descManager->GetSRVCPUHandle(m_NormalSRVIndex));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvViewDesc = {};
        srvViewDesc.Format = DXGI_FORMAT_R32_FLOAT; // Read depth as float
        srvViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvViewDesc.Texture2D.MipLevels = 1;
        srvViewDesc.Texture2D.MostDetailedMip = 0;
        srvViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        device->CreateShaderResourceView(m_DepthTexture.Get(), &srvViewDesc, descManager->GetSRVCPUHandle(m_DepthSRVIndex));
    }

    void DirectX12Prepass::CreatePipeline() {
        auto shader = ResourceManager::Get().LoadShader("Prepass", "assets/engine/shaders/dx12/prepass.hlsl", "assets/engine/shaders/dx12/prepass.hlsl");
        
        DirectX12PipelineConfig config;
        config.numRenderTargets = 2;
        config.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        config.rtvFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT; // Dummy, position isn't actually bound
        // Wait, prepass.hlsl outputs 2 targets. If I don't create the second target, I can just use DXGI_FORMAT_UNKNOWN? No, pipeline must match shader.
        // Actually I didn't create a position texture in CreateResources. Let's create it or change the shader.
        // Let's just output Normal and let the shader output position to a dummy, or just change the shader to only output Normal.
        // I will change the shader to only output Normal!
        config.numRenderTargets = 1;
        config.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        
        m_Pipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(shader.get()), config);
    }

    void DirectX12Prepass::BeginPass(ID3D12GraphicsCommandList* commandList) {
        // Transition resources to RENDER_TARGET and DEPTH_WRITE
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_NormalTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_DepthTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->ResourceBarrier(2, barriers);

        commandList->OMSetRenderTargets(1, &m_NormalRTV, FALSE, &m_DepthDSV);

        const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        commandList->ClearRenderTargetView(m_NormalRTV, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(m_DepthDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        commandList->SetGraphicsRootSignature(m_Pipeline->GetRootSignature());
        commandList->SetPipelineState(m_Pipeline->GetPipelineState());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, 1.0f };
        commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height) };
        commandList->RSSetScissorRects(1, &scissorRect);
    }

    void DirectX12Prepass::EndPass(ID3D12GraphicsCommandList* commandList) {
        // Transition resources back to PIXEL_SHADER_RESOURCE
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_NormalTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_DepthTexture.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(2, barriers);
    }

} // namespace VECTOR
