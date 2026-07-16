#pragma once

#include "Engine/Graphics/GraphicsContext.hpp"
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>

// Forward declaration of Windows HWND since we don't want to include Windows.h everywhere
struct HWND__;
typedef HWND__* HWND;

namespace VECTOR {

    class DirectX12Context : public GraphicsContext {
    public:
        DirectX12Context(HWND windowHandle, uint32_t width, uint32_t height);
        ~DirectX12Context() override;

        void Init() override;
        void SwapBuffers() override;

        // DX12 specific getters
        ID3D12Device* GetDevice() { return m_Device.Get(); }
        ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue.Get(); }
        IDXGISwapChain3* GetSwapChain() { return m_SwapChain.Get(); }
        
        static const uint8_t SWAP_CHAIN_BUFFER_COUNT = 2;

    private:
        HWND m_WindowHandle;
        uint32_t m_Width;
        uint32_t m_Height;

        Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
        Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain;
    };

} // namespace VECTOR
