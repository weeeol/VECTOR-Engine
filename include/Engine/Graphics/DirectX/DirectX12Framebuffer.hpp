#pragma once

#include "Engine/Graphics/Framebuffer.hpp"

#include <wrl.h>
#include <d3d12.h>

namespace VECTOR {

    class DirectX12Framebuffer : public Framebuffer {
    public:
        DirectX12Framebuffer(const FramebufferSpecification& spec);
        ~DirectX12Framebuffer() override;

        void Invalidate();

        void Bind() override;
        void Unbind() override;

        void Resize(uint32_t width, uint32_t height) override;

        // Return a generic ID for OpenGL compatibility if needed, 
        // but for DX12 we will use the SRV descriptor index instead
        uint32_t GetColorAttachmentRendererID() const override { return m_SrvIndex; }
        uint32_t GetDepthAttachmentRendererID() const override { return 0; }

        D3D12_GPU_DESCRIPTOR_HANDLE GetColorAttachmentSRV() const { return m_GpuSRV; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return m_RTV; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return m_DSV; }

        const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

    private:
        FramebufferSpecification m_Specification;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_ColorAttachment;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthAttachment;

        D3D12_CPU_DESCRIPTOR_HANDLE m_RTV = {};
        D3D12_CPU_DESCRIPTOR_HANDLE m_DSV = {};
        D3D12_CPU_DESCRIPTOR_HANDLE m_CpuSRV = {};
        D3D12_GPU_DESCRIPTOR_HANDLE m_GpuSRV = {};

        uint32_t m_RtvIndex = 0;
        uint32_t m_DsvIndex = 0;
        uint32_t m_SrvIndex = 0;
    };

} // namespace VECTOR
