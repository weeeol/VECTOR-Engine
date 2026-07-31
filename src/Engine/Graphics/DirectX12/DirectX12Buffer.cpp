#include "Engine/Graphics/DirectX12/DirectX12Buffer.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Core/Logger.hpp"
#include <cstring>

namespace VECTOR {

    DirectX12Buffer::DirectX12Buffer(uint64_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags) 
        : m_Size(size) {
        
        auto allocator = DirectX12Context::Get()->GetAllocator();

        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = heapType;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = size;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = flags;

        HRESULT hr = allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            initialState,
            nullptr,
            &m_Allocation,
            IID_PPV_ARGS(&m_Resource)
        );

        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create DirectX12Buffer. HRESULT: " + std::to_string(hr));
        }
    }

    DirectX12Buffer::~DirectX12Buffer() {
        m_Allocation.Reset();
        m_Resource.Reset();
    }

    void DirectX12Buffer::Map(void** data) {
        D3D12_RANGE readRange = {0, 0}; // We don't intend to read from this resource on the CPU
        HRESULT hr = m_Resource->Map(0, &readRange, data);
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to map DirectX12Buffer.");
        }
    }

    void DirectX12Buffer::Unmap() {
        D3D12_RANGE writeRange = {0, m_Size}; // Might not need to specify this perfectly if just mapping a uniform
        m_Resource->Unmap(0, &writeRange);
    }

    void DirectX12Buffer::UploadData(const void* data, size_t size) {
        void* mappedData = nullptr;
        Map(&mappedData);
        if (mappedData) {
            std::memcpy(mappedData, data, size);
            Unmap();
        }
    }

} // namespace VECTOR
