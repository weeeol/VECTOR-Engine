#pragma once
#include <directx/d3d12.h>
#include <wrl/client.h>
#include <memory>
#include "Engine/Graphics/Texture2D.hpp"

namespace VECTOR {

    class DirectX12Context;
    class DirectX12Pipeline;

    class DirectX12Prepass {
    public:
        DirectX12Prepass(DirectX12Context* context, uint32_t width, uint32_t height);
        ~DirectX12Prepass();

        void Initialize();
        void Resize(uint32_t width, uint32_t height);
        
        void BeginPass(ID3D12GraphicsCommandList* commandList, std::shared_ptr<Texture2D> outNormal, std::shared_ptr<Texture2D> outPosition, std::shared_ptr<Texture2D> outDepth);
        void EndPass(ID3D12GraphicsCommandList* commandList);

        DirectX12Pipeline* GetPipeline() const { return m_Pipeline.get(); }
        
        ID3D12Resource* GetNormalTexture() const { return m_NormalTexture.Get(); }
        ID3D12Resource* GetMotionVectorTexture() const { return m_MotionVectorTexture.Get(); }
        ID3D12Resource* GetDepthTexture() const { return m_DepthTexture.Get(); }
        
        uint32_t GetNormalSRVIndex() const { return m_NormalSRVIndex; }
        uint32_t GetMotionVectorSRVIndex() const { return m_MotionVectorSRVIndex; }
        uint32_t GetDepthSRVIndex() const { return m_DepthSRVIndex; }

    private:
        void CreateResources(uint32_t width, uint32_t height);
        void CreateDescriptors();
        void CreatePipeline();

    private:
        DirectX12Context* m_Context;
        uint32_t m_Width;
        uint32_t m_Height;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_NormalTexture;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_MotionVectorTexture;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthTexture;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

        D3D12_CPU_DESCRIPTOR_HANDLE m_NormalRTV;
        D3D12_CPU_DESCRIPTOR_HANDLE m_MotionVectorRTV;
        D3D12_CPU_DESCRIPTOR_HANDLE m_DepthDSV;
        
        uint32_t m_NormalSRVIndex = 0;
        uint32_t m_MotionVectorSRVIndex = 0;
        uint32_t m_DepthSRVIndex = 0;

        std::unique_ptr<DirectX12Pipeline> m_Pipeline;
    };

} // namespace VECTOR
