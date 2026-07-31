#pragma once

#include <directx/d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <D3D12MemAlloc.h>
#include <SDL3/SDL.h>
#include <mutex>
#include <functional>

namespace VECTOR {

    class DirectX12Context {
    public:
        DirectX12Context();
        ~DirectX12Context();

        static DirectX12Context* Get() { return s_Instance; }

        bool Initialize(SDL_Window* window);
        void Shutdown();

        Microsoft::WRL::ComPtr<IDXGIFactory7> GetFactory() const { return m_Factory; }
        Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter() const { return m_Adapter; }
        Microsoft::WRL::ComPtr<ID3D12Device9> GetDevice() const { return m_Device; }
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetGraphicsQueue() const { return m_GraphicsQueue; }
        D3D12MA::Allocator* GetAllocator() const { return m_Allocator.Get(); }
        std::mutex& GetGraphicsQueueMutex() { return m_GraphicsQueueMutex; }

        void ExecuteCommandListSync(std::function<void(ID3D12GraphicsCommandList*)> func);

    private:
        bool EnableDebugLayer();
        bool CreateFactory();
        bool PickAdapter();
        bool CreateDevice();
        bool CreateCommandQueue();
        bool CreateAllocator();

    private:
        static DirectX12Context* s_Instance;

        Microsoft::WRL::ComPtr<IDXGIFactory7> m_Factory;
        Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;
        Microsoft::WRL::ComPtr<ID3D12Device9> m_Device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_GraphicsQueue;
        std::mutex m_GraphicsQueueMutex;

        Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_Allocator;

#ifndef NDEBUG
        bool m_EnableDebugLayer = true;
#else
        bool m_EnableDebugLayer = false;
#endif
    };

} // namespace VECTOR
