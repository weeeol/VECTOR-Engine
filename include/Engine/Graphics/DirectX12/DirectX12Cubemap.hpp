#pragma once

#include <string>
#include <vector>
#include <directx/d3d12.h>
#include <wrl/client.h>
#include <D3D12MemAlloc.h>
#include "Engine/Graphics/Cubemap.hpp"

namespace VECTOR {

    class DirectX12Cubemap : public Cubemap {
    public:
        DirectX12Cubemap(const std::vector<std::string>& faces);
        ~DirectX12Cubemap();

        ID3D12Resource* GetResource() const { return m_Resource.Get(); }
        uint32_t GetDescriptorIndex() const { return m_DescriptorIndex; }

    private:
        void LoadFaces(const std::vector<std::string>& faces);

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> m_Allocation;
        uint32_t m_DescriptorIndex = -1;
    };

} // namespace VECTOR
