#include "Engine/Graphics/DirectX12/DirectX12PostProcessor.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Shader.hpp"
#include "Engine/Graphics/DirectX12/DirectX12DescriptorManager.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include "Engine/Core/Logger.hpp"
#include <directx/d3dx12.h>

namespace VECTOR {

    DirectX12PostProcessor::DirectX12PostProcessor(DirectX12Context* context, uint32_t width, uint32_t height)
        : m_Context(context), m_Width(width), m_Height(height) {
    }

    DirectX12PostProcessor::~DirectX12PostProcessor() {
        if (m_HDRTextureSRVIndex != (uint32_t)-1) {
            DirectX12DescriptorManager::Get()->FreeSRVIndex(m_HDRTextureSRVIndex);
        }
        for (auto& mip : m_BloomMips) {
            if (mip.srvIndex != (uint32_t)-1) {
                DirectX12DescriptorManager::Get()->FreeSRVIndex(mip.srvIndex);
            }
            mip.texture.Reset();
        }
        m_HDRTexture.Reset();
        m_RTVHeap.Reset();
    }

    void DirectX12PostProcessor::Initialize() {
        CreateResources(m_Width, m_Height);
        CreateDescriptors();
        CreatePipeline();
    }

    void DirectX12PostProcessor::Recreate(uint32_t width, uint32_t height) {
        if (m_Width == width && m_Height == height) return;
        m_Width = width;
        m_Height = height;

        if (m_HDRTextureSRVIndex != (uint32_t)-1) {
            DirectX12DescriptorManager::Get()->FreeSRVIndex(m_HDRTextureSRVIndex);
        }
        for (auto& mip : m_BloomMips) {
            if (mip.srvIndex != (uint32_t)-1) {
                DirectX12DescriptorManager::Get()->FreeSRVIndex(mip.srvIndex);
            }
            mip.texture.Reset();
        }
        m_BloomMips.clear();

        m_HDRTexture.Reset();
        m_RTVHeap.Reset();
        
        CreateResources(width, height);
        CreateDescriptors();
    }

    void DirectX12PostProcessor::CreateResources(uint32_t width, uint32_t height) {
        auto device = m_Context->GetDevice();

        D3D12_RESOURCE_DESC rtvDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, 1, 1);
        rtvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE rtvClearValue = {};
        rtvClearValue.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        rtvClearValue.Color[0] = 0.0f;
        rtvClearValue.Color[1] = 0.0f;
        rtvClearValue.Color[2] = 0.0f;
        rtvClearValue.Color[3] = 1.0f;

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &rtvDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &rtvClearValue,
            IID_PPV_ARGS(&m_HDRTexture)
        );
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create HDR texture");
        }

        // Bloom Mips
        m_BloomMips.resize(m_MipLevels);
        uint32_t mipWidth = width / 2;
        uint32_t mipHeight = height / 2;
        for (uint32_t i = 0; i < m_MipLevels; i++) {
            m_BloomMips[i].width = mipWidth;
            m_BloomMips[i].height = mipHeight;
            m_BloomMips[i].srvIndex = -1;

            D3D12_RESOURCE_DESC mipDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, mipWidth, mipHeight, 1, 1);
            mipDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

            hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &mipDesc,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                &rtvClearValue,
                IID_PPV_ARGS(&m_BloomMips[i].texture)
            );

            if (FAILED(hr)) {
                VECTOR_LOG_ERROR("Failed to create bloom mip texture");
            }

            mipWidth /= 2;
            mipHeight /= 2;
            if (mipWidth == 0) mipWidth = 1;
            if (mipHeight == 0) mipHeight = 1;
        }
    }

    void DirectX12PostProcessor::CreateDescriptors() {
        auto device = m_Context->GetDevice();

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 1 + m_MipLevels; // HDR + Bloom Mips
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap));

        uint32_t rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
        
        // HDR RTV
        device->CreateRenderTargetView(m_HDRTexture.Get(), nullptr, rtvHandle);
        m_HDRTextureSRVIndex = DirectX12DescriptorManager::Get()->AllocateSRVIndex();
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = DirectX12DescriptorManager::Get()->GetSRVCPUHandle(m_HDRTextureSRVIndex);
        device->CreateShaderResourceView(m_HDRTexture.Get(), nullptr, srvHandle);

        rtvHandle.ptr += rtvDescriptorSize;

        // Bloom Mip RTVs and SRVs
        for (uint32_t i = 0; i < m_MipLevels; i++) {
            m_BloomMips[i].rtvHandle = rtvHandle;
            device->CreateRenderTargetView(m_BloomMips[i].texture.Get(), nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;

            m_BloomMips[i].srvIndex = DirectX12DescriptorManager::Get()->AllocateSRVIndex();
            D3D12_CPU_DESCRIPTOR_HANDLE mipSrvHandle = DirectX12DescriptorManager::Get()->GetSRVCPUHandle(m_BloomMips[i].srvIndex);
            device->CreateShaderResourceView(m_BloomMips[i].texture.Get(), nullptr, mipSrvHandle);
        }
    }

    void DirectX12PostProcessor::CreatePipeline() {
        VECTOR_LOG_INFO("Creating DirectX 12 PostProcessor Pipeline...");
        auto shader = ResourceManager::Get().LoadShader("PostProcess", "assets/engine/shaders/dx12/postprocess.hlsl", "assets/engine/shaders/dx12/postprocess.hlsl");
        
        DirectX12PipelineConfig config;
        config.depthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        config.isDepthOnly = false;
        config.wireframe = false;
        config.numRenderTargets = 1;
        config.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; 
        config.dsvFormat = DXGI_FORMAT_UNKNOWN;
        config.emptyInputLayout = true;
        
        m_PostProcessPipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(shader.get()), config);
        
        for (int i = 0; i < 32; ++i) {
            m_PostProcessUBOPool.push_back(std::make_unique<DirectX12UniformBuffer>(16, 0));
        }

        // Bloom Pipelines
        auto downsampleShader = ResourceManager::Get().LoadShader("BloomDownsample", "assets/engine/shaders/dx12/postprocess.hlsl", "assets/engine/shaders/dx12/bloom_downsample.hlsl");
        auto upsampleShader = ResourceManager::Get().LoadShader("BloomUpsample", "assets/engine/shaders/dx12/postprocess.hlsl", "assets/engine/shaders/dx12/bloom_upsample.hlsl");

        DirectX12PipelineConfig bloomConfig = config;
        bloomConfig.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        
        // Downsample
        m_BloomDownsamplePipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(downsampleShader.get()), bloomConfig);
        
        // Upsample (Additive Blending)
        bloomConfig.blendEnable = true;
        bloomConfig.srcBlend = D3D12_BLEND_ONE;
        bloomConfig.destBlend = D3D12_BLEND_ONE;
        bloomConfig.blendOp = D3D12_BLEND_OP_ADD;
        bloomConfig.srcBlendAlpha = D3D12_BLEND_ONE;
        bloomConfig.destBlendAlpha = D3D12_BLEND_ZERO;
        bloomConfig.blendOpAlpha = D3D12_BLEND_OP_ADD;

        m_BloomUpsamplePipeline = std::make_unique<DirectX12Pipeline>(dynamic_cast<DirectX12Shader*>(upsampleShader.get()), bloomConfig);
        
        for (int i = 0; i < 128; ++i) {
            m_BloomUBOPool.push_back(std::make_unique<DirectX12UniformBuffer>(16, 0));
        }
    }

    void DirectX12PostProcessor::TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList) {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_HDRTexture.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        commandList->ResourceBarrier(1, &barrier);
    }

    void DirectX12PostProcessor::BeginMainPass(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    }

    void DirectX12PostProcessor::RenderBloom(ID3D12GraphicsCommandList* commandList) {
        if (!m_BloomEnabled || m_BloomMips.empty()) return;

        // Transition HDR to SRV
        auto hdrToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
            m_HDRTexture.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
        commandList->ResourceBarrier(1, &hdrToSRV);

        // --- Downsample Pass ---
        commandList->SetPipelineState(m_BloomDownsamplePipeline->GetPipelineState());
        commandList->SetGraphicsRootSignature(m_BloomDownsamplePipeline->GetRootSignature());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (int i = 0; i < m_MipLevels; i++) {
            // Transition current mip to Render Target
            auto toRTV = CD3DX12_RESOURCE_BARRIER::Transition(
                m_BloomMips[i].texture.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET
            );
            commandList->ResourceBarrier(1, &toRTV);

            commandList->OMSetRenderTargets(1, &m_BloomMips[i].rtvHandle, FALSE, nullptr);
            
            D3D12_VIEWPORT viewport = {0.0f, 0.0f, (float)m_BloomMips[i].width, (float)m_BloomMips[i].height, 0.0f, 1.0f};
            commandList->RSSetViewports(1, &viewport);
            D3D12_RECT scissorRect = {0, 0, (LONG)m_BloomMips[i].width, (LONG)m_BloomMips[i].height};
            commandList->RSSetScissorRects(1, &scissorRect);

            BloomDataBlock data;
            data.threshold = (i == 0) ? m_BloomThreshold : 0.0f;
            if (i == 0) {
                data.params[0] = (float)m_Width;
                data.params[1] = (float)m_Height;
                data.srcTexIndex = m_HDRTextureSRVIndex;
            } else {
                data.params[0] = (float)m_BloomMips[i-1].width;
                data.params[1] = (float)m_BloomMips[i-1].height;
                data.srcTexIndex = m_BloomMips[i-1].srvIndex;
            }

            m_BloomUBOIndex = (m_BloomUBOIndex + 1) % m_BloomUBOPool.size();
            auto ubo = m_BloomUBOPool[m_BloomUBOIndex].get();
            ubo->SetData(&data, sizeof(BloomDataBlock));
            commandList->SetGraphicsRootConstantBufferView(0, ubo->GetGPUVirtualAddress());

            commandList->DrawInstanced(3, 1, 0, 0);

            // Transition current mip to SRV for the next pass
            auto toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
                m_BloomMips[i].texture.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
            );
            commandList->ResourceBarrier(1, &toSRV);
        }

        // --- Upsample Pass ---
        commandList->SetPipelineState(m_BloomUpsamplePipeline->GetPipelineState());
        commandList->SetGraphicsRootSignature(m_BloomUpsamplePipeline->GetRootSignature());

        for (int i = m_MipLevels - 1; i > 0; i--) {
            // Target is i-1, Source is i
            // Transition target to RTV
            auto toRTV = CD3DX12_RESOURCE_BARRIER::Transition(
                m_BloomMips[i-1].texture.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET
            );
            commandList->ResourceBarrier(1, &toRTV);

            commandList->OMSetRenderTargets(1, &m_BloomMips[i-1].rtvHandle, FALSE, nullptr);
            
            D3D12_VIEWPORT viewport = {0.0f, 0.0f, (float)m_BloomMips[i-1].width, (float)m_BloomMips[i-1].height, 0.0f, 1.0f};
            commandList->RSSetViewports(1, &viewport);
            D3D12_RECT scissorRect = {0, 0, (LONG)m_BloomMips[i-1].width, (LONG)m_BloomMips[i-1].height};
            commandList->RSSetScissorRects(1, &scissorRect);

            BloomDataBlock data;
            data.params[0] = m_BloomFilterRadius;
            data.params[1] = m_BloomFilterRadius;
            data.threshold = 0.0f;
            data.srcTexIndex = m_BloomMips[i].srvIndex;

            m_BloomUBOIndex = (m_BloomUBOIndex + 1) % m_BloomUBOPool.size();
            auto ubo = m_BloomUBOPool[m_BloomUBOIndex].get();
            ubo->SetData(&data, sizeof(BloomDataBlock));
            commandList->SetGraphicsRootConstantBufferView(0, ubo->GetGPUVirtualAddress());

            commandList->DrawInstanced(3, 1, 0, 0);

            // Transition target back to SRV
            auto toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
                m_BloomMips[i-1].texture.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
            );
            commandList->ResourceBarrier(1, &toSRV);
        }
    }

    void DirectX12PostProcessor::Resolve(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE backbufferRTV, uint32_t width, uint32_t height) {
        if (m_BloomEnabled && !m_BloomMips.empty()) {
            RenderBloom(commandList);
        } else {
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                m_HDRTexture.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
            );
            commandList->ResourceBarrier(1, &barrier);
        }

        commandList->OMSetRenderTargets(1, &backbufferRTV, FALSE, nullptr);
        commandList->SetPipelineState(m_PostProcessPipeline->GetPipelineState());
        commandList->SetGraphicsRootSignature(m_PostProcessPipeline->GetRootSignature());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D12_VIEWPORT viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
        commandList->RSSetViewports(1, &viewport);
        D3D12_RECT scissorRect = {0, 0, (LONG)width, (LONG)height};
        commandList->RSSetScissorRects(1, &scissorRect);

        PostProcessData ppData;
        ppData.exposure = m_Exposure;
        ppData.hdrTexIndex = m_HDRTextureSRVIndex;
        if (m_BloomEnabled && !m_BloomMips.empty()) {
            ppData.bloomTexIndex = m_BloomMips[0].srvIndex;
            ppData.useBloom = 1;
            ppData.bloomIntensity = m_BloomIntensity;
        } else {
            ppData.bloomTexIndex = -1;
            ppData.useBloom = 0;
            ppData.bloomIntensity = 1.0f;
        }
        ppData.padding[0] = 0;
        ppData.padding[1] = 0;
        ppData.padding[2] = 0;
        
        m_PostProcessUBOIndex = (m_PostProcessUBOIndex + 1) % m_PostProcessUBOPool.size();
        auto ubo = m_PostProcessUBOPool[m_PostProcessUBOIndex].get();
        ubo->SetData(&ppData, sizeof(PostProcessData));
        commandList->SetGraphicsRootConstantBufferView(0, ubo->GetGPUVirtualAddress());

        // Global SRV Heap is already bound by the renderer
        commandList->DrawInstanced(3, 1, 0, 0);
    }

} // namespace VECTOR
