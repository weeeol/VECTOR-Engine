#pragma once

#include "Engine/Graphics/GraphicsContext.hpp"
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <vector>

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

        static DirectX12Context* Get();

        // DX12 specific getters
        ID3D12Device* GetDevice() { return m_Device.Get(); }
        ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue.Get(); }
        IDXGISwapChain3* GetSwapChain() { return m_SwapChain.Get(); }
        
        static const uint8_t SWAP_CHAIN_BUFFER_COUNT = 2;

        // RTV and Synchronization getters
        ID3D12Resource* GetCurrentBackBuffer() { return m_RenderTargets[m_FrameIndex].Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;

        // Command List getters
        ID3D12CommandAllocator* GetCommandAllocator() { return m_CommandAllocator.Get(); }
        ID3D12GraphicsCommandList* GetCommandList() { return m_CommandList.Get(); }
        ID3D12RootSignature* GetRootSignature() { return m_RootSignature.Get(); }
        ID3D12DescriptorHeap* GetSrvHeap() { return m_SrvHeap.Get(); }

        // Returns a CPU and GPU handle for a new SRV/CBV descriptor, and increments the offset
        void AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle, uint32_t& outDescriptorIndex);
        void FreeDescriptor(uint32_t index);

        // Descriptor allocation for RTV and DSV
        void AllocateRTV(D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle, uint32_t& outDescriptorIndex);
        void AllocateDSV(D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle, uint32_t& outDescriptorIndex);

        // Uploads data to the CBV ring buffer and returns the GPU virtual address
        D3D12_GPU_VIRTUAL_ADDRESS UploadConstantBuffer(const void* data, size_t size);

        void FlushCommandQueue();

        ID3D12DescriptorHeap* GetDSVHeap() { return m_DsvHeap.Get(); }

    private:
        void WaitForPreviousFrame();

        HWND m_WindowHandle;
        uint32_t m_Width;
        uint32_t m_Height;

        Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
        Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_RenderTargets[SWAP_CHAIN_BUFFER_COUNT];
        uint32_t m_RtvDescriptorSize;
        uint32_t m_CurrentRtvOffset = SWAP_CHAIN_BUFFER_COUNT; // First two are swapchain
        uint32_t m_FrameIndex;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SrvHeap;
        uint32_t m_SrvDescriptorSize;
        uint32_t m_CurrentSrvOffset = 0;
        std::vector<uint32_t> m_FreeSrvIndices;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DsvHeap;
        uint32_t m_DsvDescriptorSize;
        uint32_t m_CurrentDsvOffset = 0;
        
        Microsoft::WRL::ComPtr<ID3D12Resource> m_CbvUploadBuffer;
        uint8_t* m_CbvDataBegin = nullptr;
        size_t m_CbvOffset = 0;
        static const size_t MAX_CBV_UPLOAD_SIZE = 1024 * 1024 * 8; // 8 MB

        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        uint64_t m_FenceValue;
        HANDLE m_FenceEvent;
    };

} // namespace VECTOR
