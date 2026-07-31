#pragma once

#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include <directx/d3d12.h>
#include <wrl/client.h>
#include <mutex>
#include <vector>

namespace VECTOR {

    class DirectX12DescriptorManager {
    public:
        DirectX12DescriptorManager();
        ~DirectX12DescriptorManager();

        static DirectX12DescriptorManager* Get() { return s_Instance; }

        bool Initialize();
        void Shutdown();

        ID3D12DescriptorHeap* GetSRVHeap() const { return m_SRVHeap.Get(); }
        ID3D12DescriptorHeap* GetSamplerHeap() const { return m_SamplerHeap.Get(); }

        uint32_t AllocateSRVIndex();
        void FreeSRVIndex(uint32_t index);

        uint32_t AllocateSamplerIndex();
        void FreeSamplerIndex(uint32_t index);

        D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle(uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(uint32_t index) const;

        D3D12_CPU_DESCRIPTOR_HANDLE GetSamplerCPUHandle(uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerGPUHandle(uint32_t index) const;

    private:
        static DirectX12DescriptorManager* s_Instance;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SamplerHeap;

        uint32_t m_SRVDescriptorSize = 0;
        uint32_t m_SamplerDescriptorSize = 0;

        static const uint32_t s_MaxSRVDescriptors = 1000000;
        static const uint32_t s_MaxSamplerDescriptors = 2048;

        std::mutex m_SRVMutex;
        uint32_t m_NextSRVIndex = 0;
        std::vector<uint32_t> m_FreeSRVIndices;

        std::mutex m_SamplerMutex;
        uint32_t m_NextSamplerIndex = 0;
        std::vector<uint32_t> m_FreeSamplerIndices;
    };

} // namespace VECTOR
