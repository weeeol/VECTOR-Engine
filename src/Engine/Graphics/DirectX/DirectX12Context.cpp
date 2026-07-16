#include "Engine/Graphics/DirectX/DirectX12Context.hpp"
#include "Engine/Core/Logger.hpp"

// We must include Windows headers here to interact with the OS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace VECTOR {

    static DirectX12Context* s_Instance = nullptr;

    DirectX12Context* DirectX12Context::Get() {
        return s_Instance;
    }

    DirectX12Context::DirectX12Context(HWND windowHandle, uint32_t width, uint32_t height)
        : m_WindowHandle(windowHandle), m_Width(width), m_Height(height), m_FrameIndex(0), m_FenceValue(0), m_FenceEvent(nullptr) {
        s_Instance = this;
        if (!m_WindowHandle) {
            VECTOR_LOG_ERROR("DirectX12Context: Window handle is null!");
        }
    }

    DirectX12Context::~DirectX12Context() {
        WaitForPreviousFrame();
        if (m_FenceEvent) {
            CloseHandle(m_FenceEvent);
        }
        if (s_Instance == this) {
            s_Instance = nullptr;
        }
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
                m_CommandQueue.Get(),
                m_WindowHandle,
                &swapChainDesc,
                nullptr,
                nullptr,
                &swapChain))) {
            VECTOR_LOG_ERROR("Failed to create DXGI Swap Chain.");
            return;
        }
        swapChain.As(&m_SwapChain);
        m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

        // Create RTV Descriptor Heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 64; // 2 for SwapChain, rest for FBOs
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (FAILED(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap)))) {
            VECTOR_LOG_ERROR("Failed to create RTV Descriptor Heap.");
            return;
        }
        m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Create DSV Heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 64; // For Depth Buffers
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (FAILED(m_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap)))) {
            VECTOR_LOG_ERROR("Failed to create DSV Descriptor Heap.");
            return;
        }
        m_DsvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        // Create RTVs
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT n = 0; n < SWAP_CHAIN_BUFFER_COUNT; n++) {
            if (FAILED(m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n])))) {
                VECTOR_LOG_ERROR("Failed to get swap chain buffer.");
                return;
            }
            m_Device->CreateRenderTargetView(m_RenderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += m_RtvDescriptorSize;
        }

        // Create Command Allocator
        if (FAILED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)))) {
            VECTOR_LOG_ERROR("Failed to create Command Allocator.");
            return;
        }

        // Create SRV Descriptor Heap
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1024; // Simple limit for all textures/CBVs
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(m_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvHeap)))) {
            VECTOR_LOG_ERROR("Failed to create SRV Descriptor Heap.");
            return;
        }
        m_SrvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Create CBV Upload Buffer (Ring Buffer)
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC cbvBufferDesc = {};
        cbvBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbvBufferDesc.Width = MAX_CBV_UPLOAD_SIZE;
        cbvBufferDesc.Height = 1;
        cbvBufferDesc.DepthOrArraySize = 1;
        cbvBufferDesc.MipLevels = 1;
        cbvBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        cbvBufferDesc.SampleDesc.Count = 1;
        cbvBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        cbvBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        if (FAILED(m_Device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &cbvBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_CbvUploadBuffer)))) {
            VECTOR_LOG_ERROR("Failed to create CBV Upload Buffer.");
            return;
        }

        // Map the CBV buffer and keep it mapped
        D3D12_RANGE readRange = {0, 0}; // We do not intend to read from this resource on the CPU
        m_CbvUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_CbvDataBegin));

        // Create Command List
        if (FAILED(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)))) {
            VECTOR_LOG_ERROR("Failed to create Command List.");
            return;
        }

        // Create Synchronization Objects
        if (FAILED(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)))) {
            VECTOR_LOG_ERROR("Failed to create fence.");
            return;
        }
        m_FenceValue = 1;
        m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_FenceEvent == nullptr) {
            if (FAILED(HRESULT_FROM_WIN32(GetLastError()))) {
                VECTOR_LOG_ERROR("Failed to create fence event.");
                return;
            }
        }

        // Create Global Root Signature
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        // 1. Descriptor Table for SRV (Texture)
        D3D12_DESCRIPTOR_RANGE1 ranges[1] = {};
        ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[0].NumDescriptors = 1;
        ranges[0].BaseShaderRegister = 0;
        ranges[0].RegisterSpace = 0;
        ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
        ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER1 rootParameters[2] = {};
        
        // Parameter 0: CBV (Constant Buffer) for matrices/lighting
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        rootParameters[0].Descriptor.RegisterSpace = 0;
        rootParameters[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // Parameter 1: Descriptor Table for SRV
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[1].DescriptorTable.pDescriptorRanges = ranges;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSignatureDesc.Desc_1_1.NumParameters = 2;
        rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
        rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
        rootSignatureDesc.Desc_1_1.pStaticSamplers = &sampler;
        rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error);
        if (FAILED(hr)) {
            if (error) VECTOR_LOG_ERROR("Root Signature Serialization Error: " + std::string((char*)error->GetBufferPointer()));
            return;
        }

        hr = m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create Root Signature.");
            return;
        }

        VECTOR_LOG_INFO("DirectX 12 Context Initialized successfully.");
    }

    void DirectX12Context::SwapBuffers() {
        if (m_SwapChain) {
            m_SwapChain->Present(1, 0); // VSync enabled
            WaitForPreviousFrame();
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Context::GetCurrentRTV() const {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += m_FrameIndex * m_RtvDescriptorSize;
        return rtvHandle;
    }

    void DirectX12Context::WaitForPreviousFrame() {
        const uint64_t fence = m_FenceValue;
        if (FAILED(m_CommandQueue->Signal(m_Fence.Get(), fence))) return;
        m_FenceValue++;

        if (m_Fence->GetCompletedValue() < fence) {
            if (FAILED(m_Fence->SetEventOnCompletion(fence, m_FenceEvent))) return;
            WaitForSingleObject(m_FenceEvent, INFINITE);
        }

        m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

        // GPU has finished, safe to reset command allocator and list for the next frame
        if (m_CommandAllocator) {
            m_CommandAllocator->Reset();
            m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);
        }
    }

    void DirectX12Context::FlushCommandQueue() {
        // Execute the command list
        m_CommandList->Close();
        ID3D12CommandList* ppCommandLists[] = { m_CommandList.Get() };
        m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Signal fence and wait
        m_FenceValue++;
        if (FAILED(m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue))) return;
        if (m_Fence->GetCompletedValue() < m_FenceValue) {
            if (FAILED(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent))) return;
            WaitForSingleObject(m_FenceEvent, INFINITE);
        }

        // Reset command allocator and list so it can be recorded again
        m_CommandAllocator->Reset();
        m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);
    }

    void DirectX12Context::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle, uint32_t& outDescriptorIndex) {
        if (m_CurrentSrvOffset >= 1024) {
            VECTOR_LOG_ERROR("DirectX12Context::AllocateDescriptor - Maximum SRV descriptors reached!");
            return;
        }
        outDescriptorIndex = m_CurrentSrvOffset;
        
        outCpuHandle = m_SrvHeap->GetCPUDescriptorHandleForHeapStart();
        outCpuHandle.ptr += outDescriptorIndex * m_SrvDescriptorSize;
        
        outGpuHandle = m_SrvHeap->GetGPUDescriptorHandleForHeapStart();
        outGpuHandle.ptr += outDescriptorIndex * m_SrvDescriptorSize;
        
        m_CurrentSrvOffset++;
    }

    void DirectX12Context::AllocateRTV(D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle, uint32_t& outDescriptorIndex) {
        if (m_CurrentRtvOffset >= 64) {
            VECTOR_LOG_ERROR("DirectX12Context::AllocateRTV - Maximum RTV descriptors reached!");
            return;
        }
        outDescriptorIndex = m_CurrentRtvOffset;
        
        outCpuHandle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
        outCpuHandle.ptr += outDescriptorIndex * m_RtvDescriptorSize;
        
        m_CurrentRtvOffset++;
    }

    void DirectX12Context::AllocateDSV(D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle, uint32_t& outDescriptorIndex) {
        if (m_CurrentDsvOffset >= 64) {
            VECTOR_LOG_ERROR("DirectX12Context::AllocateDSV - Maximum DSV descriptors reached!");
            return;
        }
        outDescriptorIndex = m_CurrentDsvOffset;
        
        outCpuHandle = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
        outCpuHandle.ptr += outDescriptorIndex * m_DsvDescriptorSize;
        
        m_CurrentDsvOffset++;
    }

    D3D12_GPU_VIRTUAL_ADDRESS DirectX12Context::UploadConstantBuffer(const void* data, size_t size) {
        // CBVs must be 256-byte aligned
        size_t alignedSize = (size + 255) & ~255;
        
        if (m_CbvOffset + alignedSize > MAX_CBV_UPLOAD_SIZE) {
            // Reset the ring buffer if we run out of space.
            // WARNING: In a real engine, we must ensure the GPU has finished using the old data before resetting.
            // For now, we will wait for idle.
            FlushCommandQueue();
            m_CbvOffset = 0;
        }

        uint8_t* destination = m_CbvDataBegin + m_CbvOffset;
        memcpy(destination, data, size);

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_CbvUploadBuffer->GetGPUVirtualAddress() + m_CbvOffset;
        m_CbvOffset += alignedSize;

        return gpuAddress;
    }

} // namespace VECTOR
