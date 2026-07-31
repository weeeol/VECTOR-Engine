#include "Engine/Graphics/DirectX12/DirectX12DescriptorManager.hpp"
#include "Engine/Core/Logger.hpp"
#include <directx/d3dx12.h>
#include <cassert>
#include <string>

namespace VECTOR {

    DirectX12DescriptorManager* DirectX12DescriptorManager::s_Instance = nullptr;

    DirectX12DescriptorManager::DirectX12DescriptorManager() {
        assert(s_Instance == nullptr && "DirectX12DescriptorManager already exists!");
        s_Instance = this;
    }

    DirectX12DescriptorManager::~DirectX12DescriptorManager() {
        Shutdown();
        s_Instance = nullptr;
    }

    bool DirectX12DescriptorManager::Initialize() {
        auto device = DirectX12Context::Get()->GetDevice();

        // Create SRV/CBV/UAV Heap
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = s_MaxSRVDescriptors;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRVHeap));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create SRV Descriptor Heap. HRESULT: " + std::to_string(hr));
            return false;
        }
        m_SRVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Create Sampler Heap
        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        samplerHeapDesc.NumDescriptors = s_MaxSamplerDescriptors;
        samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        hr = device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_SamplerHeap));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create Sampler Descriptor Heap. HRESULT: " + std::to_string(hr));
            return false;
        }
        m_SamplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        VECTOR_LOG_INFO("DirectX 12 Descriptor Manager Initialized.");
        return true;
    }

    void DirectX12DescriptorManager::Shutdown() {
        m_SRVHeap.Reset();
        m_SamplerHeap.Reset();
    }

    uint32_t DirectX12DescriptorManager::AllocateSRVIndex() {
        std::lock_guard<std::mutex> lock(m_SRVMutex);
        if (!m_FreeSRVIndices.empty()) {
            uint32_t index = m_FreeSRVIndices.back();
            m_FreeSRVIndices.pop_back();
            return index;
        }
        assert(m_NextSRVIndex < s_MaxSRVDescriptors && "Ran out of SRV descriptors!");
        return m_NextSRVIndex++;
    }

    void DirectX12DescriptorManager::FreeSRVIndex(uint32_t index) {
        std::lock_guard<std::mutex> lock(m_SRVMutex);
        m_FreeSRVIndices.push_back(index);
    }

    uint32_t DirectX12DescriptorManager::AllocateSamplerIndex() {
        std::lock_guard<std::mutex> lock(m_SamplerMutex);
        if (!m_FreeSamplerIndices.empty()) {
            uint32_t index = m_FreeSamplerIndices.back();
            m_FreeSamplerIndices.pop_back();
            return index;
        }
        assert(m_NextSamplerIndex < s_MaxSamplerDescriptors && "Ran out of Sampler descriptors!");
        return m_NextSamplerIndex++;
    }

    void DirectX12DescriptorManager::FreeSamplerIndex(uint32_t index) {
        std::lock_guard<std::mutex> lock(m_SamplerMutex);
        m_FreeSamplerIndices.push_back(index);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DirectX12DescriptorManager::GetSRVCPUHandle(uint32_t index) const {
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_SRVHeap->GetCPUDescriptorHandleForHeapStart(), index, m_SRVDescriptorSize);
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DirectX12DescriptorManager::GetSRVGPUHandle(uint32_t index) const {
        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_SRVHeap->GetGPUDescriptorHandleForHeapStart(), index, m_SRVDescriptorSize);
        return handle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DirectX12DescriptorManager::GetSamplerCPUHandle(uint32_t index) const {
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_SamplerHeap->GetCPUDescriptorHandleForHeapStart(), index, m_SamplerDescriptorSize);
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DirectX12DescriptorManager::GetSamplerGPUHandle(uint32_t index) const {
        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_SamplerHeap->GetGPUDescriptorHandleForHeapStart(), index, m_SamplerDescriptorSize);
        return handle;
    }

} // namespace VECTOR
