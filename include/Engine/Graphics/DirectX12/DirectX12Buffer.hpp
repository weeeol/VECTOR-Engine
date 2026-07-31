#pragma once
#include <directx/d3d12.h>
#include <wrl/client.h>
#include <D3D12MemAlloc.h>
#include <cstdint>

namespace VECTOR {

    class DirectX12Buffer {
    public:
        DirectX12Buffer(uint64_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
        ~DirectX12Buffer();

        DirectX12Buffer(const DirectX12Buffer&) = delete;
        DirectX12Buffer& operator=(const DirectX12Buffer&) = delete;

        ID3D12Resource* GetResource() const { return m_Resource.Get(); }
        D3D12MA::Allocation* GetAllocation() const { return m_Allocation.Get(); }
        uint64_t GetSize() const { return m_Size; }

        void Map(void** data);
        void Unmap();

        void UploadData(const void* data, size_t size);

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> m_Allocation;
        uint64_t m_Size = 0;
    };

} // namespace VECTOR
