#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Core/Logger.hpp"
#include <cassert>
#include <string>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

namespace VECTOR {

    DirectX12Context* DirectX12Context::s_Instance = nullptr;

    DirectX12Context::DirectX12Context() {
        assert(s_Instance == nullptr && "DirectX12Context already exists!");
        s_Instance = this;
    }

    DirectX12Context::~DirectX12Context() {
        Shutdown();
        s_Instance = nullptr;
    }

    bool DirectX12Context::Initialize(SDL_Window* window) {
        // We will enable the debug layer but prevent it from crashing by querying the info queue.
        m_EnableDebugLayer = true;
        if (m_EnableDebugLayer) {
            if (!EnableDebugLayer()) {
                VECTOR_LOG_WARN("Failed to enable DirectX 12 debug layer.");
            }
        }

        if (!CreateFactory()) return false;
        if (!PickAdapter()) return false;

        // Log adapter info
        DXGI_ADAPTER_DESC1 desc;
        m_Adapter->GetDesc1(&desc);
        std::wstring ws(desc.Description);
        std::string adapterName;
        for (wchar_t c : ws) adapterName += static_cast<char>(c);
        VECTOR_LOG_INFO("DirectX 12 Context Initialized. Adapter: " + adapterName);

        if (!CreateDevice()) return false;
        if (!CreateCommandQueue()) return false;
        if (!CreateAllocator()) return false;

        return true;
    }

    void DirectX12Context::Shutdown() {
        if (!m_Device) return;
        m_Allocator.Reset();
        m_GraphicsQueue.Reset();
        m_Device.Reset();
        m_Adapter.Reset();
        m_Factory.Reset();
        VECTOR_LOG_INFO("DirectX 12 Context Shutdown.");
    }

    void DirectX12Context::ExecuteCommandListSync(std::function<void(ID3D12GraphicsCommandList*)> func) {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
        m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
        m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList));

        func(cmdList.Get());

        cmdList->Close();

        ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
        m_GraphicsQueue->ExecuteCommandLists(1, ppCommandLists);

        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        
        m_GraphicsQueue->Signal(fence.Get(), 1);
        if (fence->GetCompletedValue() < 1) {
            fence->SetEventOnCompletion(1, eventHandle);
            WaitForSingleObject(eventHandle, INFINITE);
        }
        CloseHandle(eventHandle);
    }

    bool DirectX12Context::EnableDebugLayer() {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            
            // Enable GPU-based validation if possible
            Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
            if (SUCCEEDED(debugController.As(&debugController1))) {
                // GPU-based validation has a massive performance overhead. 
                // Enable only when specifically debugging shader/resource issues.
                debugController1->SetEnableGPUBasedValidation(FALSE);
            }
            return true;
        }
        return false;
    }

    bool DirectX12Context::CreateFactory() {
        UINT factoryFlags = 0;
        if (m_EnableDebugLayer) {
            factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }

        HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_Factory));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create DXGI Factory. HRESULT: " + std::to_string(hr));
            return false;
        }

        return true;
    }

    bool DirectX12Context::PickAdapter() {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_Factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)); ++adapterIndex) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr))) {
                if (SUCCEEDED(adapter.As(&m_Adapter))) {
                    return true;
                }
            }
        }

        VECTOR_LOG_ERROR("Failed to find a suitable DirectX 12 adapter.");
        return false;
    }

    bool DirectX12Context::CreateDevice() {
        HRESULT hr = D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create DirectX 12 Device. HRESULT: " + std::to_string(hr));
            return false;
        }

        if (m_EnableDebugLayer) {
            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(m_Device.As(&infoQueue))) {
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, FALSE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, FALSE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);
            }
        }
        return true;
    }

    bool DirectX12Context::CreateCommandQueue() {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        HRESULT hr = m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_GraphicsQueue));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create Command Queue. HRESULT: " + std::to_string(hr));
            return false;
        }

        return true;
    }

    bool DirectX12Context::CreateAllocator() {
        D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
        allocatorDesc.pDevice = m_Device.Get();
        allocatorDesc.pAdapter = m_Adapter.Get();
        // Since we are using Agility SDK (likely via directx-headers), we don't need any special D3D12MA flags right now unless we want D3D12MA_ALLOCATOR_FLAG_NONE

        HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, &m_Allocator);
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create D3D12MemoryAllocator. HRESULT: " + std::to_string(hr));
            return false;
        }

        return true;
    }

} // namespace VECTOR
