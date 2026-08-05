#include "Engine/Graphics/DirectX12/DirectX12TAA.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Pipeline.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Shader.hpp"
#include "Engine/Graphics/DirectX12/DirectX12DescriptorManager.hpp"
#include "Engine/Graphics/DirectX12/DirectX12UniformBuffer.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <directx/d3dx12.h>

namespace VECTOR {

    DirectX12TAA::DirectX12TAA(DirectX12Context* context, uint32_t width, uint32_t height)
        : m_Context(context), m_Width(width), m_Height(height) {
    }

    DirectX12TAA::~DirectX12TAA() {
        for (int i = 0; i < 2; ++i) {
            m_HistoryTextures[i].Reset();
        }
        m_RTVHeap.Reset();
    }

    void DirectX12TAA::Initialize() {
        CreateResources();
        CreateDescriptors();
        CreatePipeline();

        m_TAADataBuffer = std::make_unique<DirectX12UniformBuffer>(sizeof(TAADataBlock), 2);
    }

    void DirectX12TAA::Resize(uint32_t width, uint32_t height) {
        if (m_Width == width && m_Height == height) return;
        m_Width = width;
        m_Height = height;

        for (int i = 0; i < 2; ++i) {
            m_HistoryTextures[i].Reset();
        }
        
        CreateResources();
        CreateDescriptors();
    }

    void DirectX12TAA::CreateResources() {
        auto device = m_Context->GetDevice();

        D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, m_Width, m_Height, 1, 1);
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        
        for (int i = 0; i < 2; ++i) {
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(&m_HistoryTextures[i])
            );
        }
    }

    void DirectX12TAA::CreateDescriptors() {
        auto device = m_Context->GetDevice();
        auto descManager = DirectX12DescriptorManager::Get();

        // RTV Heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 2;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap));

        uint32_t rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        
        for (int i = 0; i < 2; ++i) {
            m_HistoryRTVs[i] = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
            m_HistoryRTVs[i].ptr += (SIZE_T)i * rtvDescriptorSize;
            device->CreateRenderTargetView(m_HistoryTextures[i].Get(), nullptr, m_HistoryRTVs[i]);
            
            if (m_HistorySRVIndices[i] == 0) m_HistorySRVIndices[i] = descManager->AllocateSRVIndex();
            
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(m_HistoryTextures[i].Get(), &srvDesc, descManager->GetSRVCPUHandle(m_HistorySRVIndices[i]));
        }
    }

    void DirectX12TAA::CreatePipeline() {
        auto shader = ResourceManager::Get().LoadShader("TAA", "assets/engine/shaders/dx12/taa.hlsl", "assets/engine/shaders/dx12/taa.hlsl");
        
        DirectX12PipelineConfig config;
        config.numRenderTargets = 1;
        config.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        config.isDepthOnly = false;
        config.dsvFormat = DXGI_FORMAT_UNKNOWN;
        config.depthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        config.emptyInputLayout = true;
        
        m_Pipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(shader.get()), config);
    }

    void DirectX12TAA::Resolve(ID3D12GraphicsCommandList* commandList, 
                               uint32_t currentFrameSRVIndex, 
                               uint32_t motionVectorSRVIndex,
                               uint32_t depthSRVIndex) {
        
        uint32_t previousHistoryIndex = m_CurrentHistoryIndex;
        m_CurrentHistoryIndex = 1 - m_CurrentHistoryIndex;
        
        // Transition current history to RENDER_TARGET
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_HistoryTextures[m_CurrentHistoryIndex].Get(), 
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        commandList->ResourceBarrier(1, &barrier);

        commandList->OMSetRenderTargets(1, &m_HistoryRTVs[m_CurrentHistoryIndex], FALSE, nullptr);

        commandList->SetGraphicsRootSignature(m_Pipeline->GetRootSignature());
        commandList->SetPipelineState(m_Pipeline->GetPipelineState());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_TAAData.screenSize = glm::vec2((float)m_Width, (float)m_Height);
        m_TAAData.currentFrameTexIndex = currentFrameSRVIndex;
        m_TAAData.historyTexIndex = m_HistorySRVIndices[previousHistoryIndex];
        m_TAAData.motionVectorTexIndex = motionVectorSRVIndex;
        m_TAAData.depthTexIndex = depthSRVIndex;
        
        m_TAADataBuffer->SetData(&m_TAAData, sizeof(TAADataBlock));
        commandList->SetGraphicsRootConstantBufferView(1, m_TAADataBuffer->GetGPUVirtualAddress());

        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, 1.0f };
        commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height) };
        commandList->RSSetScissorRects(1, &scissorRect);

        commandList->DrawInstanced(3, 1, 0, 0);

        // Transition current history back to PIXEL_SHADER_RESOURCE
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_HistoryTextures[m_CurrentHistoryIndex].Get(), 
            D3D12_RESOURCE_STATE_RENDER_TARGET, 
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
        commandList->ResourceBarrier(1, &barrier);
    }

} // namespace VECTOR
