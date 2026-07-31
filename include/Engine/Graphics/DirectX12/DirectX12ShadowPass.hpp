#pragma once

#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Pipeline.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Shader.hpp"
#include <directx/d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <cstdint>

namespace VECTOR {

    class DirectX12ShadowPass {
    public:
        DirectX12ShadowPass(DirectX12Context* context);
        ~DirectX12ShadowPass();

        DirectX12ShadowPass(const DirectX12ShadowPass&) = delete;
        DirectX12ShadowPass& operator=(const DirectX12ShadowPass&) = delete;

        void Initialize();
        void Shutdown();

        void BeginPass(ID3D12GraphicsCommandList* commandList);
        void EndPass(ID3D12GraphicsCommandList* commandList);

        uint32_t GetShadowMapSize() const { return SHADOW_MAP_SIZE; }
        uint32_t GetSRVIndex() const { return m_SRVIndex; }
        DirectX12Pipeline* GetPipeline() const { return m_Pipeline.get(); }
        ID3D12Resource* GetShadowMapResource() const { return m_ShadowMap.Get(); }

    private:
        void CreateResources();
        void CreatePipeline();

        DirectX12Context* m_Context = nullptr;
        const uint32_t SHADOW_MAP_SIZE = 2048;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_ShadowMap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

        uint32_t m_SRVIndex = 0;
        
        std::unique_ptr<DirectX12Pipeline> m_Pipeline;
        std::unique_ptr<DirectX12Shader> m_Shader;
    };

} // namespace VECTOR
