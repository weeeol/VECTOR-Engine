#include "Engine/Graphics/DirectX12/DirectX12ShadowPass.hpp"
#include "Engine/Graphics/DirectX12/DirectX12DescriptorManager.hpp"
#include "Engine/Core/Logger.hpp"
#include <directx/d3dx12.h>

namespace VECTOR {

    DirectX12ShadowPass::DirectX12ShadowPass(DirectX12Context* context)
        : m_Context(context) {
    }

    DirectX12ShadowPass::~DirectX12ShadowPass() {
        Shutdown();
    }

    void DirectX12ShadowPass::Initialize() {
        CreateResources();
        CreatePipeline();
    }

    void DirectX12ShadowPass::Shutdown() {
        m_Pipeline.reset();
        m_Shader.reset();
        m_ShadowMap.Reset();
        m_DSVHeap.Reset();
    }

    void DirectX12ShadowPass::CreateResources() {
        auto device = m_Context->GetDevice();

        // Create the Depth/Stencil descriptor heap (only needs 1 DSV)
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        
        HRESULT hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("DirectX12ShadowPass: Failed to create DSV descriptor heap.");
            return;
        }

        // Create the Shadow Map resource
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Alignment = 0;
        texDesc.Width = SHADOW_MAP_SIZE;
        texDesc.Height = SHADOW_MAP_SIZE;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

        hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &clearValue,
            IID_PPV_ARGS(&m_ShadowMap)
        );

        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("DirectX12ShadowPass: Failed to create shadow map texture.");
            return;
        }
        
        m_ShadowMap->SetName(L"Shadow Map");

        // Create DSV
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        device->CreateDepthStencilView(m_ShadowMap.Get(), &dsvDesc, m_DSVHeap->GetCPUDescriptorHandleForHeapStart());

        // Create SRV in the Global Descriptor Heap
        m_SRVIndex = DirectX12DescriptorManager::Get()->AllocateSRVIndex();
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;

        device->CreateShaderResourceView(
            m_ShadowMap.Get(), 
            &srvDesc, 
            DirectX12DescriptorManager::Get()->GetSRVCPUHandle(m_SRVIndex)
        );
    }

    void DirectX12ShadowPass::CreatePipeline() {
        m_Shader = std::make_unique<DirectX12Shader>("assets/engine/shaders/dx12/depth.hlsl", "assets/engine/shaders/dx12/depth.hlsl");
        m_Pipeline = std::make_unique<DirectX12Pipeline>(m_Shader.get(), D3D12_COMPARISON_FUNC_LESS_EQUAL, true);
    }

    void DirectX12ShadowPass::BeginPass(ID3D12GraphicsCommandList* commandList) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_ShadowMap.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        
        commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(SHADOW_MAP_SIZE), static_cast<float>(SHADOW_MAP_SIZE), 0.0f, 1.0f };
        D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(SHADOW_MAP_SIZE), static_cast<LONG>(SHADOW_MAP_SIZE) };
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);
        
        commandList->SetGraphicsRootSignature(m_Pipeline->GetRootSignature());
        commandList->SetPipelineState(m_Pipeline->GetPipelineState());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void DirectX12ShadowPass::EndPass(ID3D12GraphicsCommandList* commandList) {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_ShadowMap.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }

} // namespace VECTOR
