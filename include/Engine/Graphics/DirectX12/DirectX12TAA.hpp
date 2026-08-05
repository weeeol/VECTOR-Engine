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

    class DirectX12TAA {
    public:
        DirectX12TAA(DirectX12Context* context, uint32_t width, uint32_t height);
        ~DirectX12TAA();

        void Initialize();
        void Resize(uint32_t width, uint32_t height);
        
        void Resolve(ID3D12GraphicsCommandList* commandList, 
                      uint32_t currentFrameSRVIndex, 
                      uint32_t motionVectorSRVIndex,
                      uint32_t depthSRVIndex);

        ID3D12Resource* GetResolvedTexture() const { return m_HistoryTextures[m_CurrentHistoryIndex].Get(); }
        uint32_t GetResolvedSRVIndex() const { return m_HistorySRVIndices[m_CurrentHistoryIndex]; }

    private:
        void CreateResources();
        void CreateDescriptors();
        void CreatePipeline();

    private:
        DirectX12Context* m_Context;
        uint32_t m_Width;
        uint32_t m_Height;

        // Ping-pong history buffers
        Microsoft::WRL::ComPtr<ID3D12Resource> m_HistoryTextures[2];
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE m_HistoryRTVs[2];
        uint32_t m_HistorySRVIndices[2] = {0, 0};
        
        uint32_t m_CurrentHistoryIndex = 0;

        std::unique_ptr<DirectX12Pipeline> m_Pipeline;

        struct TAADataBlock {
            glm::vec2 screenSize;
            int currentFrameTexIndex = -1;
            int historyTexIndex = -1;
            int motionVectorTexIndex = -1;
            int depthTexIndex = -1;
            int padding[2];
        };

        TAADataBlock m_TAAData;
        std::unique_ptr<DirectX12UniformBuffer> m_TAADataBuffer;
    };

} // namespace VECTOR
