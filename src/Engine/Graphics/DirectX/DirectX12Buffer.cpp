#include "Engine/Graphics/DirectX/DirectX12Buffer.hpp"
#include "Engine/Graphics/DirectX/DirectX12Context.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    // Helper function for creating buffers
    static void CreateDX12Buffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* data, uint32_t size, Microsoft::WRL::ComPtr<ID3D12Resource>& outBuffer, Microsoft::WRL::ComPtr<ID3D12Resource>& outUploadBuffer) {
        D3D12_HEAP_PROPERTIES defaultHeapProps = {};
        defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        defaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        defaultHeapProps.CreationNodeMask = 1;
        defaultHeapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Alignment = 0;
        bufferDesc.Width = size;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.SampleDesc.Quality = 0;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        device->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&outBuffer)
        );

        D3D12_HEAP_PROPERTIES uploadHeapProps = defaultHeapProps;
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&outUploadBuffer)
        );

        // Copy data to upload heap
        void* mappedData = nullptr;
        outUploadBuffer->Map(0, nullptr, &mappedData);
        memcpy(mappedData, data, size);
        outUploadBuffer->Unmap(0, nullptr);

        // Record copy command
        cmdList->CopyBufferRegion(outBuffer.Get(), 0, outUploadBuffer.Get(), 0, size);

        // Transition default buffer to generic read
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = outBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);
    }

    // --- DirectX12VertexBuffer ---
    DirectX12VertexBuffer::DirectX12VertexBuffer(const void* vertices, uint32_t size) {
        VECTOR_LOG_INFO("Creating DirectX12VertexBuffer");
        auto context = DirectX12Context::Get();
        if (!context) {
            VECTOR_LOG_ERROR("DirectX12Context is null!");
            return;
        }

        auto device = context->GetDevice();
        auto cmdList = context->GetCommandList();

        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
        CreateDX12Buffer(device, cmdList, vertices, size, m_VertexBuffer, uploadBuffer);

        // Flush command list to execute the copy and wait for completion, so uploadBuffer can be safely destroyed
        context->FlushCommandQueue();

        m_BufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_BufferView.SizeInBytes = size;
        m_BufferView.StrideInBytes = 0; // Set properly during Bind
    }

    DirectX12VertexBuffer::~DirectX12VertexBuffer() {
    }

    void DirectX12VertexBuffer::Bind() const {
        auto context = DirectX12Context::Get();
        if (!context) return;

        auto cmdList = context->GetCommandList();
        
        // We calculate stride based on layout type dynamically for now
        uint32_t stride = 0;
        switch (m_LayoutType) {
            case BufferLayoutType::Mesh3D: stride = sizeof(float) * 8; break; // pos(3), normal(3), tex(2)
            case BufferLayoutType::Quad2D: stride = sizeof(float) * 4; break; // pos(2), tex(2)
            default: stride = sizeof(float) * 8; break;
        }

        D3D12_VERTEX_BUFFER_VIEW view = m_BufferView;
        view.StrideInBytes = stride;

        cmdList->IASetVertexBuffers(0, 1, &view);
    }

    void DirectX12VertexBuffer::Unbind() const {
    }

    // --- DirectX12IndexBuffer ---
    DirectX12IndexBuffer::DirectX12IndexBuffer(const uint32_t* indices, uint32_t count)
        : m_Count(count) {
        VECTOR_LOG_INFO("Creating DirectX12IndexBuffer");
        auto context = DirectX12Context::Get();
        if (!context) {
            VECTOR_LOG_ERROR("DirectX12Context is null!");
            return;
        }

        auto device = context->GetDevice();
        auto cmdList = context->GetCommandList();

        uint32_t size = count * sizeof(uint32_t);
        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
        CreateDX12Buffer(device, cmdList, indices, size, m_IndexBuffer, uploadBuffer);

        // Flush command list to execute the copy
        context->FlushCommandQueue();

        m_BufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
        m_BufferView.SizeInBytes = size;
        m_BufferView.Format = DXGI_FORMAT_R32_UINT;
    }

    DirectX12IndexBuffer::~DirectX12IndexBuffer() {
    }

    void DirectX12IndexBuffer::Bind() const {
        auto context = DirectX12Context::Get();
        if (!context) return;

        auto cmdList = context->GetCommandList();
        cmdList->IASetIndexBuffer(&m_BufferView);
    }

    void DirectX12IndexBuffer::Unbind() const {
    }

} // namespace VECTOR
