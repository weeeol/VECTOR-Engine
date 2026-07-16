#include "Engine/Graphics/DirectX/DirectX12Context.hpp"
#include "Engine/Core/Logger.hpp"

// We must include Windows headers here to interact with the OS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace VECTOR {

    DirectX12Context::DirectX12Context(HWND windowHandle, uint32_t width, uint32_t height)
        : m_WindowHandle(windowHandle), m_Width(width), m_Height(height) {
        if (!m_WindowHandle) {
            VECTOR_LOG_ERROR("DirectX12Context: Window handle is null!");
        }
    }

    DirectX12Context::~DirectX12Context() {
        // ComPtrs will automatically release resources
    }

    void DirectX12Context::Init() {
        VECTOR_LOG_INFO("Creating DirectX 12 Context...");

        UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
        // Enable the D3D12 debug layer
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
#endif

        if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_Factory)))) {
            VECTOR_LOG_ERROR("Failed to create DXGI Factory.");
            return;
        }

        // Create the D3D12 device
        if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device)))) {
            VECTOR_LOG_ERROR("Failed to create D3D12 Device.");
            return;
        }

        // Create the command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        if (FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)))) {
            VECTOR_LOG_ERROR("Failed to create D3D12 Command Queue.");
            return;
        }

        // Create swap chain
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
        swapChainDesc.Width = m_Width;
        swapChainDesc.Height = m_Height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
        if (FAILED(m_Factory->CreateSwapChainForHwnd(
                m_CommandQueue.Get(),    // Swap chain needs the queue so that it can force a flush on it
                m_WindowHandle,
                &swapChainDesc,
                nullptr,
                nullptr,
                &swapChain))) {
            VECTOR_LOG_ERROR("Failed to create DXGI Swap Chain.");
            return;
        }

        swapChain.As(&m_SwapChain);

        // This would be followed by creating descriptor heaps, RTVs, and sync fences.
        // We will implement those parts as we refine the renderer API integration.
        VECTOR_LOG_INFO("DirectX 12 Context Initialized successfully.");
    }

    void DirectX12Context::SwapBuffers() {
        if (m_SwapChain) {
            // Present the frame
            m_SwapChain->Present(1, 0);
        }
    }

} // namespace VECTOR
