#include "Engine/Graphics/DirectX/DirectX12Texture2D.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/DirectX/DirectX12Context.hpp"
#include <SDL_image.h>

namespace VECTOR {

    DirectX12Texture2D::DirectX12Texture2D(const std::string& path)
        : m_FilePath(path), m_Width(0), m_Height(0), m_BPP(0) {
        VECTOR_LOG_INFO("Creating DirectX12Texture2D from " + path);
        
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            VECTOR_LOG_ERROR("Failed to load texture: " + path);
            return;
        }

        // Convert to RGBA32
        SDL_Surface* formattedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        if (!formattedSurface) {
            VECTOR_LOG_ERROR("Failed to convert texture surface: " + path);
            return;
        }

        m_Width = formattedSurface->w;
        m_Height = formattedSurface->h;
        m_BPP = 4;

        auto context = DirectX12Context::Get();
        if (!context) {
            SDL_FreeSurface(formattedSurface);
            return;
        }

        auto device = context->GetDevice();
        auto cmdList = context->GetCommandList();

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Alignment = 0;
        textureDesc.Width = m_Width;
        textureDesc.Height = m_Height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES defaultHeapProps = {};
        defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        if (FAILED(device->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_Texture)))) {
            VECTOR_LOG_ERROR("Failed to create Texture2D resource.");
            SDL_FreeSurface(formattedSurface);
            return;
        }

        // Create upload heap
        UINT64 uploadBufferSize = 0;
        device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC uploadDesc = {};
        uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDesc.Width = uploadBufferSize;
        uploadDesc.Height = 1;
        uploadDesc.DepthOrArraySize = 1;
        uploadDesc.MipLevels = 1;
        uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadDesc.SampleDesc.Count = 1;
        uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_UploadBuffer));

        // Copy data to upload heap
        void* mappedData = nullptr;
        m_UploadBuffer->Map(0, nullptr, &mappedData);

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
        UINT numRows;
        UINT64 rowSizeInBytes;
        UINT64 totalBytes;
        device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

        // Copy row by row to respect pitch
        const uint8_t* src = (const uint8_t*)formattedSurface->pixels;
        uint8_t* dest = (uint8_t*)mappedData;
        for (UINT y = 0; y < numRows; ++y) {
            memcpy(dest + footprint.Offset + y * footprint.Footprint.RowPitch, src + y * formattedSurface->pitch, rowSizeInBytes);
        }

        m_UploadBuffer->Unmap(0, nullptr);
        SDL_FreeSurface(formattedSurface);

        // Record copy command
        D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
        dstLoc.pResource = m_Texture.Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
        srcLoc.pResource = m_UploadBuffer.Get();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint = footprint;

        cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        // Transition to pixel shader resource
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_Texture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        // Removed FlushCommandQueue to avoid resetting the command list mid-frame!

        // Create SRV
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        context->AllocateDescriptor(m_CpuDescriptorHandle, m_GpuDescriptorHandle, m_DescriptorIndex);
        device->CreateShaderResourceView(m_Texture.Get(), &srvDesc, m_CpuDescriptorHandle);
    }

    DirectX12Texture2D::DirectX12Texture2D(const void* pixels, int width, int height)
        : m_FilePath("memory"), m_Width(width), m_Height(height), m_BPP(4) {
        VECTOR_LOG_INFO("Creating DirectX12Texture2D from memory");

        auto context = DirectX12Context::Get();
        if (!context) return;
        auto device = context->GetDevice();
        auto cmdList = context->GetCommandList();

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Alignment = 0;
        textureDesc.Width = m_Width;
        textureDesc.Height = m_Height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES defaultHeapProps = {};
        defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        if (FAILED(device->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_Texture)))) {
            VECTOR_LOG_ERROR("Failed to create Texture2D resource.");
            return;
        }

        UINT64 uploadBufferSize = 0;
        device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC uploadDesc = {};
        uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDesc.Width = uploadBufferSize;
        uploadDesc.Height = 1;
        uploadDesc.DepthOrArraySize = 1;
        uploadDesc.MipLevels = 1;
        uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadDesc.SampleDesc.Count = 1;
        uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_UploadBuffer));

        void* mappedData = nullptr;
        m_UploadBuffer->Map(0, nullptr, &mappedData);

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
        UINT numRows;
        UINT64 rowSizeInBytes;
        UINT64 totalBytes;
        device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

        const uint8_t* src = (const uint8_t*)pixels;
        uint8_t* dest = (uint8_t*)mappedData;
        int pitch = width * 4;
        for (UINT y = 0; y < numRows; ++y) {
            memcpy(dest + footprint.Offset + y * footprint.Footprint.RowPitch, src + y * pitch, rowSizeInBytes);
        }

        m_UploadBuffer->Unmap(0, nullptr);

        D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
        dstLoc.pResource = m_Texture.Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
        srcLoc.pResource = m_UploadBuffer.Get();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint = footprint;

        cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_Texture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        // Removed FlushCommandQueue!

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        context->AllocateDescriptor(m_CpuDescriptorHandle, m_GpuDescriptorHandle, m_DescriptorIndex);
        device->CreateShaderResourceView(m_Texture.Get(), &srvDesc, m_CpuDescriptorHandle);
    }

    DirectX12Texture2D::~DirectX12Texture2D() {
        if (m_DescriptorIndex != (uint32_t)-1) {
            auto context = DirectX12Context::Get();
            if (context) {
                context->FreeDescriptor(m_DescriptorIndex);
            }
        }
    }

    void DirectX12Texture2D::Bind(unsigned int slot) const {
        // Will be handled via Descriptor Tables and Root Signatures in DX12
    }

    void DirectX12Texture2D::Unbind() const {
    }

} // namespace VECTOR
