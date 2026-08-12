#include "Engine/Graphics/DirectX12/DirectX12Prepass.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Pipeline.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Shader.hpp"
#include "Engine/Graphics/DirectX12/DirectX12DescriptorManager.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Texture2D.hpp"
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
    }

    void DirectX12Prepass::CreateDescriptors() {
        auto device = m_Context->GetDevice();

        // RTV Heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 2; // Normal + Position/MotionVector
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap));

        uint32_t rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_NormalRTV = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
        
        m_MotionVectorRTV = m_NormalRTV;
        m_MotionVectorRTV.ptr += rtvDescriptorSize;

        // DSV Heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap));

        m_DepthDSV = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
    }

    void DirectX12Prepass::CreatePipeline() {
        auto shader = ResourceManager::Get().LoadShader("Prepass", "assets/engine/shaders/dx12/prepass.hlsl", "assets/engine/shaders/dx12/prepass.hlsl");
        
        DirectX12PipelineConfig config;
        config.numRenderTargets = 2;
        config.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        config.rtvFormats[1] = DXGI_FORMAT_R16G16_FLOAT; // Motion Vectors
        
        m_Pipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(shader.get()), config);
    }

    void DirectX12Prepass::BeginPass(ID3D12GraphicsCommandList* commandList, std::shared_ptr<Texture2D> outNormal, std::shared_ptr<Texture2D> outPosition, std::shared_ptr<Texture2D> outDepth) {
        auto dxNormal = std::dynamic_pointer_cast<DirectX12Texture2D>(outNormal);
        auto dxPosition = std::dynamic_pointer_cast<DirectX12Texture2D>(outPosition);
        auto dxDepth = std::dynamic_pointer_cast<DirectX12Texture2D>(outDepth);
        
        if (!dxNormal || !dxPosition || !dxDepth) {
            VECTOR_LOG_ERROR("DirectX12Prepass received invalid textures from RenderGraph!");
            return;
        }

        m_NormalTexture = dxNormal->GetResource();
        m_MotionVectorTexture = dxPosition->GetResource(); // Map Position to MotionVector slot for now
        m_DepthTexture = dxDepth->GetResource();
        
        m_NormalSRVIndex = dxNormal->GetDescriptorIndex();
        m_MotionVectorSRVIndex = dxPosition->GetDescriptorIndex();
        m_DepthSRVIndex = dxDepth->GetDescriptorIndex();

        // Update RTV/DSV
        auto device = m_Context->GetDevice();
        device->CreateRenderTargetView(m_NormalTexture.Get(), nullptr, m_NormalRTV);
        device->CreateRenderTargetView(m_MotionVectorTexture.Get(), nullptr, m_MotionVectorRTV);
        
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvViewDesc = {};
        dsvViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvViewDesc.Texture2D.MipSlice = 0;
        device->CreateDepthStencilView(m_DepthTexture.Get(), &dsvViewDesc, m_DepthDSV);

        // Transition resources to RENDER_TARGET and DEPTH_WRITE handled by Graph? Wait, Graph transitions to RENDER_TARGET / DEPTH_WRITE?
        // Right now our graph hasn't been fully updated to issue barriers.
        // We will issue manual barriers for now.
        // REMOVED: RenderGraph now handles these transitions!

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] = { m_NormalRTV, m_MotionVectorRTV };
        commandList->OMSetRenderTargets(2, rtvHandles, FALSE, &m_DepthDSV);

        const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        commandList->ClearRenderTargetView(m_NormalRTV, clearColor, 0, nullptr);
        commandList->ClearRenderTargetView(m_MotionVectorRTV, clearColor, 0, nullptr);
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
        if (!m_NormalTexture) return;
        // REMOVED: RenderGraph now handles transition back to SRV!
    }
} // namespace VECTOR
