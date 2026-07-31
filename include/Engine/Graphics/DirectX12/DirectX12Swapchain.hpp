#pragma once

#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include <directx/d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>

namespace VECTOR {

    class DirectX12Swapchain {
    public:
        DirectX12Swapchain(DirectX12Context* context, SDL_Window* window, uint32_t width, uint32_t height);
        ~DirectX12Swapchain();

        void Recreate(uint32_t width, uint32_t height);
        
        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }

        uint32_t AcquireNextImage();
        void Present(bool vSync);
        
        Microsoft::WRL::ComPtr<IDXGISwapChain4> GetSwapchain() const { return m_Swapchain; }
        DXGI_FORMAT GetImageFormat() const { return m_ImageFormat; }
        
        size_t GetImageCount() const { return m_BackBuffers.size(); }
        ID3D12Resource* GetBackBuffer(int index) const { return m_BackBuffers[index].Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(int index) const;

        DXGI_FORMAT GetDepthFormat() const { return m_DepthFormat; }
        ID3D12Resource* GetDepthBuffer() const { return m_DepthBuffer.Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthBufferDSV() const;

    private:
        void Create();
        void Cleanup();

        void CreateSwapchain();
        void CreateRTVs();
        void CreateDSVs();

    private:
        DirectX12Context* m_Context;
        SDL_Window* m_Window;
        uint32_t m_Width, m_Height;

        Microsoft::WRL::ComPtr<IDXGISwapChain4> m_Swapchain;
        DXGI_FORMAT m_ImageFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        DXGI_FORMAT m_DepthFormat = DXGI_FORMAT_D32_FLOAT;

        static const uint32_t s_FrameCount = 3;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_BackBuffers;
        
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
        uint32_t m_RTVDescriptorSize = 0;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> m_DepthAllocation;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
    };

} // namespace VECTOR
