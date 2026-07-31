#include "Engine/Graphics/DirectX12/DirectX12Swapchain.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Core/Logger.hpp"
#include <SDL3/SDL.h>
#include <directx/d3dx12.h>
#include <string>

namespace VECTOR {

    DirectX12Swapchain::DirectX12Swapchain(DirectX12Context* context, SDL_Window* window, uint32_t width, uint32_t height)
        : m_Context(context), m_Window(window), m_Width(width), m_Height(height) {
        Create();
    }

    DirectX12Swapchain::~DirectX12Swapchain() {
        Cleanup();
    }

    void DirectX12Swapchain::Recreate(uint32_t width, uint32_t height) {
        m_Width = width;
        m_Height = height;
        Cleanup();
        Create();
    }

    uint32_t DirectX12Swapchain::AcquireNextImage() {
        return m_Swapchain->GetCurrentBackBufferIndex();
    }

    void DirectX12Swapchain::Present(bool vSync) {
        UINT syncInterval = vSync ? 1 : 0;
        HRESULT hr = m_Swapchain->Present(syncInterval, 0);
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to present DirectX 12 Swapchain. HRESULT: " + std::to_string(hr));
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Swapchain::GetBackBufferRTV(int index) const {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), index, m_RTVDescriptorSize);
        return rtvHandle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Swapchain::GetDepthBufferDSV() const {
        return m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
    }

    void DirectX12Swapchain::Create() {
        CreateSwapchain();
        CreateRTVs();
        CreateDSVs();
    }

    void DirectX12Swapchain::Cleanup() {
        m_BackBuffers.clear();
        m_RTVHeap.Reset();
        m_DepthBuffer.Reset();
        m_DepthAllocation.Reset();
        m_DSVHeap.Reset();
        m_Swapchain.Reset();
    }

    void DirectX12Swapchain::CreateSwapchain() {
        auto factory = m_Context->GetFactory();
        auto queue = m_Context->GetGraphicsQueue();

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = s_FrameCount;
        swapChainDesc.Width = m_Width;
        swapChainDesc.Height = m_Height;
        swapChainDesc.Format = m_ImageFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(m_Window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        if (!hwnd) {
            VECTOR_LOG_ERROR("Failed to get HWND from SDL3 window");
            return;
        }

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
        HRESULT hr = factory->CreateSwapChainForHwnd(
            queue.Get(),
            hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
        );

        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create DXGI Swapchain. HRESULT: " + std::to_string(hr));
            return;
        }

        // Disable ALT+ENTER full screen as SDL handles it
        factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

        swapChain1.As(&m_Swapchain);
    }

    void DirectX12Swapchain::CreateRTVs() {
        auto device = m_Context->GetDevice();

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = s_FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create RTV Descriptor Heap. HRESULT: " + std::to_string(hr));
            return;
        }
        m_RTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        m_BackBuffers.resize(s_FrameCount);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

        for (uint32_t i = 0; i < s_FrameCount; ++i) {
            m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i]));
            device->CreateRenderTargetView(m_BackBuffers[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_RTVDescriptorSize);
        }
    }

    void DirectX12Swapchain::CreateDSVs() {
        auto device = m_Context->GetDevice();
        auto allocator = m_Context->GetAllocator();

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap));

        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC depthDesc = {};
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Alignment = 0;
        depthDesc.Width = m_Width;
        depthDesc.Height = m_Height;
        depthDesc.DepthOrArraySize = 1;
        depthDesc.MipLevels = 1;
        depthDesc.Format = m_DepthFormat;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = m_DepthFormat;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        HRESULT hr = allocator->CreateResource(
            &allocDesc,
            &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            &m_DepthAllocation,
            IID_PPV_ARGS(&m_DepthBuffer)
        );

        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create Depth Buffer. HRESULT: " + std::to_string(hr));
            return;
        }

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = m_DepthFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsvDesc, m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
    }

} // namespace VECTOR
