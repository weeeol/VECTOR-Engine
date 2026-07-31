#pragma once
#include <directx/d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <glm/glm.hpp>
#include <vector>

namespace VECTOR {

    class DirectX12Context;
    class DirectX12Pipeline;
    class DirectX12UniformBuffer;

    class DirectX12SSAO {
    public:
        DirectX12SSAO(DirectX12Context* context, uint32_t width, uint32_t height);
        ~DirectX12SSAO();

        void Initialize();
        void Resize(uint32_t width, uint32_t height);
        
        void Generate(ID3D12GraphicsCommandList* commandList, 
                      uint32_t normalSRVIndex, 
                      uint32_t depthSRVIndex,
                      const glm::mat4& projection, 
                      const glm::mat4& view);

        ID3D12Resource* GetSSAOTexture() const { return m_BlurTexture.Get(); }
        uint32_t GetSSAOSRVIndex() const { return m_BlurSRVIndex; }

    private:
        void CreateResources();
        void CreateNoiseTexture();
        void CreateDescriptors();
        void CreatePipelines();

        void GenerateKernel();

    private:
        DirectX12Context* m_Context;
        uint32_t m_Width;
        uint32_t m_Height;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_SSAOTexture;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_BlurTexture;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_NoiseTexture;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_NoiseUploadBuffer;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVHeap;

        D3D12_CPU_DESCRIPTOR_HANDLE m_SSAORTV;
        D3D12_CPU_DESCRIPTOR_HANDLE m_BlurRTV;

        uint32_t m_SSAOSRVIndex = 0;
        uint32_t m_BlurSRVIndex = 0;
        uint32_t m_NoiseSRVIndex = 0;
        
        std::unique_ptr<DirectX12Pipeline> m_SSAOPipeline;
        std::unique_ptr<DirectX12Pipeline> m_BlurPipeline;

        struct SSAODataBlock {
            glm::mat4 projection;
            glm::mat4 invProjection;
            glm::vec4 samples[64];
            float radius = 0.5f;
            float bias = 0.025f;
            glm::vec2 screenSize;
            int normalTexIndex = -1;
            int depthTexIndex = -1;
            int noiseTexIndex = -1;
            int ssaoTexIndex = -1;
        };

        SSAODataBlock m_SSAOData;
        std::unique_ptr<DirectX12UniformBuffer> m_SSAODataBuffer;
    };

} // namespace VECTOR
