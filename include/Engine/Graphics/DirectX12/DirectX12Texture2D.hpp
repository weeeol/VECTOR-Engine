#pragma once

#include "Engine/Graphics/Texture2D.hpp"
#include <directx/d3d12.h>
#include <wrl/client.h>
#include <D3D12MemAlloc.h>
#include <memory>
#include <string>

namespace VECTOR {

    class DirectX12Texture2D : public Texture2D {
    public:
        DirectX12Texture2D(const std::string& path);
        DirectX12Texture2D(uint32_t width, uint32_t height, void* data, uint32_t size);
        virtual ~DirectX12Texture2D();

        virtual void Bind(unsigned int slot = 0) const override;
        virtual void Unbind() const override {}

        virtual unsigned int GetID() const override { return m_DescriptorIndex; }
        virtual int GetWidth() const override { return m_Width; }
        virtual int GetHeight() const override { return m_Height; }

        ID3D12Resource* GetResource() const { return m_Resource.Get(); }
        uint32_t GetDescriptorIndex() const { return m_DescriptorIndex; }

    private:
        void LoadFromFile(const std::string& path);
        void CreateTexture(void* data, uint32_t size);

    private:
        int m_Width = 0, m_Height = 0, m_Channels = 0;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> m_Allocation;
        uint32_t m_DescriptorIndex = -1;
        std::string m_Path;
    };

} // namespace VECTOR
