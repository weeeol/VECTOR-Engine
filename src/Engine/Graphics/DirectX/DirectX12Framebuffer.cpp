#include "Engine/Graphics/DirectX/DirectX12Framebuffer.hpp"
#include "Engine/Graphics/DirectX/DirectX12Context.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    DirectX12Framebuffer::DirectX12Framebuffer(const FramebufferSpecification& spec)
        : m_Specification(spec) {
        VECTOR_LOG_INFO("Creating DirectX12Framebuffer");
        Invalidate();
    }

    DirectX12Framebuffer::~DirectX12Framebuffer() {
    }

    void DirectX12Framebuffer::Invalidate() {
        if (m_Specification.Width == 0 || m_Specification.Height == 0) return;

        auto context = DirectX12Context::Get();
        auto device = context->GetDevice();

        // 1. Create Color Attachment
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC colorDesc = {};
        colorDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        colorDesc.Alignment = 0;
        colorDesc.Width = m_Specification.Width;
        colorDesc.Height = m_Specification.Height;
        colorDesc.DepthOrArraySize = 1;
        colorDesc.MipLevels = 1;
        colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        colorDesc.SampleDesc.Count = 1;
        colorDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;

        if (FAILED(device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &colorDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&m_ColorAttachment)))) {
            VECTOR_LOG_ERROR("Failed to create Framebuffer Color Attachment");
            return;
        }

        // Allocate RTV and create view
        context->AllocateRTV(m_RTV, m_RtvIndex);
        device->CreateRenderTargetView(m_ColorAttachment.Get(), nullptr, m_RTV);

        // Allocate SRV and create view
        context->AllocateDescriptor(m_CpuSRV, m_GpuSRV, m_SrvIndex);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_ColorAttachment.Get(), &srvDesc, m_CpuSRV);

        // 2. Create Depth Attachment
        D3D12_RESOURCE_DESC depthDesc = colorDesc;
        depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        
        D3D12_CLEAR_VALUE depthClear = {};
        depthClear.Format = DXGI_FORMAT_D32_FLOAT;
        depthClear.DepthStencil.Depth = 1.0f;
        depthClear.DepthStencil.Stencil = 0;

        if (FAILED(device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear, IID_PPV_ARGS(&m_DepthAttachment)))) {
            VECTOR_LOG_ERROR("Failed to create Framebuffer Depth Attachment");
            return;
        }

        // Allocate DSV and create view
        context->AllocateDSV(m_DSV, m_DsvIndex);
        device->CreateDepthStencilView(m_DepthAttachment.Get(), nullptr, m_DSV);
    }

    void DirectX12Framebuffer::Bind() {
        auto context = DirectX12Context::Get();
        auto cmdList = context->GetCommandList();

        // Transition from SRV to RTV
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_ColorAttachment.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        cmdList->OMSetRenderTargets(1, &m_RTV, FALSE, &m_DSV);

        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_Specification.Width), static_cast<float>(m_Specification.Height), 0.0f, 1.0f };
        cmdList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_Specification.Width), static_cast<LONG>(m_Specification.Height) };
        cmdList->RSSetScissorRects(1, &scissorRect);
    }

    void DirectX12Framebuffer::Unbind() {
        auto context = DirectX12Context::Get();
        auto cmdList = context->GetCommandList();

        // Transition from RTV to SRV for sampling
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_ColorAttachment.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        
        // Wait, Unbind does not automatically restore backbuffer, the Renderer::Clear will do that on the next pass or Present
    }

    void DirectX12Framebuffer::Resize(uint32_t width, uint32_t height) {
        m_Specification.Width = width;
        m_Specification.Height = height;
        Invalidate();
    }

} // namespace VECTOR
