#pragma once

#include "Engine/Graphics/Texture2D.hpp"
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <cstdint>

namespace VECTOR {

    class DirectX12Texture2D : public Texture2D {
    public:
        DirectX12Texture2D(const std::string& path);
        DirectX12Texture2D(const void* pixels, int width, int height);
        virtual ~DirectX12Texture2D();

        virtual void Bind(unsigned int slot = 0) const override;
        virtual void Unbind() const override;

        virtual unsigned int GetID() const override { return 0; } // IDs are an OpenGL concept
        virtual int GetWidth() const override { return m_Width; }
        virtual int GetHeight() const override { return m_Height; }

        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle() const { return m_GpuDescriptorHandle; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuffer;
        int m_Width, m_Height, m_BPP;
        std::string m_FilePath;

        D3D12_CPU_DESCRIPTOR_HANDLE m_CpuDescriptorHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE m_GpuDescriptorHandle;
        uint32_t m_DescriptorIndex;
    };

} // namespace VECTOR
