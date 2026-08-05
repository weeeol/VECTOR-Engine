#pragma once

#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Pipeline.hpp"
#include "Engine/Graphics/DirectX12/DirectX12UniformBuffer.hpp"
#include <memory>
#include <wrl.h>
#include <vector>

namespace VECTOR {

    class DirectX12PostProcessor {
    public:
        DirectX12PostProcessor(DirectX12Context* context, uint32_t width, uint32_t height);
        ~DirectX12PostProcessor();

        void Initialize();
        void Recreate(uint32_t width, uint32_t height);

        void TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList);
        void TransitionHDRToSRV(ID3D12GraphicsCommandList* commandList);

        // Sets the HDR offscreen render target as the current RTV
        void BeginMainPass(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);
        
        // Resolves the HDR offscreen render target to the backbuffer using Tone Mapping and Gamma Correction
        void Resolve(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE backbufferRTV, uint32_t width, uint32_t height, uint32_t inputSRVIndex);

        D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return m_RTVHeap->GetCPUDescriptorHandleForHeapStart(); }
        uint32_t GetHDRTextureSRVIndex() const { return m_HDRTextureSRVIndex; }

        float GetExposure() const { return m_Exposure; }
        void SetExposure(float exposure) { m_Exposure = exposure; }

        bool m_BloomEnabled = true;

    private:
        void CreateResources(uint32_t width, uint32_t height);
        void CreateDescriptors();
        void CreatePipeline();
        void RenderBloom(ID3D12GraphicsCommandList* commandList, uint32_t inputSRVIndex);

        DirectX12Context* m_Context;
        uint32_t m_Width;
        uint32_t m_Height;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_HDRTexture;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
        uint32_t m_HDRTextureSRVIndex = 0;

        std::unique_ptr<DirectX12Pipeline> m_PostProcessPipeline;

        struct PostProcessData {
            float exposure = 1.0f;
            int hdrTexIndex = -1;
            int bloomTexIndex = -1;
            int useBloom = 0;
            float bloomIntensity = 1.5f; // Add intensity, defaulting to 1.5
            int padding[3] = {0, 0, 0};
        };

        struct BloomMip {
            Microsoft::WRL::ComPtr<ID3D12Resource> texture;
            uint32_t width;
            uint32_t height;
            uint32_t srvIndex;
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
        };

        struct BloomDataBlock {
            float params[2]; // srcResolution (downsample) or filterRadius (upsample)
            float threshold;
            int srcTexIndex;
        };

        float m_Exposure = 1.0f;
        uint32_t m_MipLevels = 6;
        float m_BloomThreshold = 0.5f;
        float m_BloomFilterRadius = 0.005f;
        float m_BloomIntensity = 2.0f; // Added intensity parameter

        std::vector<BloomMip> m_BloomMips;
        std::unique_ptr<DirectX12Pipeline> m_BloomDownsamplePipeline;
        std::unique_ptr<DirectX12Pipeline> m_BloomUpsamplePipeline;
        
        std::vector<std::unique_ptr<DirectX12UniformBuffer>> m_BloomUBOPool;
        uint32_t m_BloomUBOIndex = 0;
        
        std::vector<std::unique_ptr<DirectX12UniformBuffer>> m_PostProcessUBOPool;
        uint32_t m_PostProcessUBOIndex = 0;
    };

} // namespace VECTOR
