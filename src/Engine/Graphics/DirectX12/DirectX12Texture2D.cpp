#include "Engine/Graphics/DirectX12/DirectX12Texture2D.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12DescriptorManager.hpp"
#include "Engine/Core/Logger.hpp"
#include <stb_image.h>

namespace VECTOR {

    DirectX12Texture2D::DirectX12Texture2D(const std::string& path)
        : m_Path(path) {
        LoadFromFile(path);
    }

    DirectX12Texture2D::DirectX12Texture2D(uint32_t width, uint32_t height, void* data, uint32_t size)
        : m_Width(width), m_Height(height) {
        CreateTexture(data, size);
    }

    DirectX12Texture2D::~DirectX12Texture2D() {
        if (m_DescriptorIndex != -1) {
            DirectX12DescriptorManager::Get()->FreeSRVIndex(m_DescriptorIndex);
        }
    }

    void DirectX12Texture2D::Bind(unsigned int slot) const {
        // In bindless SM 6.6, binding a texture just means passing m_DescriptorIndex to a constant buffer.
        // We don't need to do anything here.
    }

    void DirectX12Texture2D::LoadFromFile(const std::string& path) {
        stbi_set_flip_vertically_on_load(1);
        stbi_uc* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, 4);
        
        if (data) {
            CreateTexture(data, m_Width * m_Height * 4);
            stbi_image_free(data);
        } else {
            VECTOR_LOG_ERROR("Failed to load texture: " + path);
        }
    }

    void DirectX12Texture2D::CreateTexture(void* data, uint32_t size) {
        auto allocator = DirectX12Context::Get()->GetAllocator();
        auto device = DirectX12Context::Get()->GetDevice();

        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = m_Width;
        resourceDesc.Height = m_Height;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            &m_Allocation,
            IID_PPV_ARGS(&m_Resource)
        );

        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create Texture2D resource.");
            return;
        }

        // Allocate descriptor
        m_DescriptorIndex = DirectX12DescriptorManager::Get()->AllocateSRVIndex();
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = resourceDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, DirectX12DescriptorManager::Get()->GetSRVCPUHandle(m_DescriptorIndex));
        
        // Upload data
        if (data && size > 0) {
            UINT numRows;
            UINT64 rowSizeInBytes;
            UINT64 totalBytes;
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
            device->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

            Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
            D3D12MA::ALLOCATION_DESC uploadAllocDesc = {};
            uploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC uploadDesc = {};
            uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            uploadDesc.Alignment = 0;
            uploadDesc.Width = totalBytes;
            uploadDesc.Height = 1;
            uploadDesc.DepthOrArraySize = 1;
            uploadDesc.MipLevels = 1;
            uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
            uploadDesc.SampleDesc.Count = 1;
            uploadDesc.SampleDesc.Quality = 0;
            uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            Microsoft::WRL::ComPtr<D3D12MA::Allocation> uploadAllocation;
            hr = allocator->CreateResource(
                &uploadAllocDesc,
                &uploadDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                &uploadAllocation,
                IID_PPV_ARGS(&uploadBuffer)
            );

            if (SUCCEEDED(hr)) {
                uint8_t* pData;
                uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));
                
                uint8_t* pSrc = static_cast<uint8_t*>(data);
                for (UINT y = 0; y < numRows; ++y) {
                    memcpy(pData + footprint.Offset + y * footprint.Footprint.RowPitch, pSrc + y * rowSizeInBytes, rowSizeInBytes);
                }
                
                uploadBuffer->Unmap(0, nullptr);

                DirectX12Context::Get()->ExecuteCommandListSync([&](ID3D12GraphicsCommandList* cmdList) {
                    D3D12_TEXTURE_COPY_LOCATION dst = {};
                    dst.pResource = m_Resource.Get();
                    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    dst.SubresourceIndex = 0;

                    D3D12_TEXTURE_COPY_LOCATION src = {};
                    src.pResource = uploadBuffer.Get();
                    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                    src.PlacedFootprint = footprint;
                    
                    cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
                    
                    D3D12_RESOURCE_BARRIER barrier = {};
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier.Transition.pResource = m_Resource.Get();
                    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    cmdList->ResourceBarrier(1, &barrier);
                });
            }
        } else {
            // Just transition if no data
            DirectX12Context::Get()->ExecuteCommandListSync([&](ID3D12GraphicsCommandList* cmdList) {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = m_Resource.Get();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                cmdList->ResourceBarrier(1, &barrier);
            });
        }
    }

    DirectX12Texture2D::DirectX12Texture2D(uint32_t width, uint32_t height, TextureFormat format) 
        : m_Width(width), m_Height(height) {
        auto allocator = DirectX12Context::Get()->GetAllocator();
        auto device = DirectX12Context::Get()->GetDevice();

        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = width;
        resourceDesc.Height = height;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        
        bool isDepth = (format == TextureFormat::Depth32F);
        if (isDepth) {
            resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
        } else {
            switch (format) {
                case TextureFormat::RGBA16F: resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
                case TextureFormat::RGBA32F: resourceDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
                case TextureFormat::RG16F: resourceDesc.Format = DXGI_FORMAT_R16G16_FLOAT; break;
                case TextureFormat::RG32F: resourceDesc.Format = DXGI_FORMAT_R32G32_FLOAT; break;
                default: resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
            }
        }
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = isDepth ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        HRESULT hr = allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            isDepth ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_RENDER_TARGET,
            nullptr,
            &m_Allocation,
            IID_PPV_ARGS(&m_Resource)
        );

        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create transient RenderGraph resource.");
            return;
        }

        m_DescriptorIndex = DirectX12DescriptorManager::Get()->AllocateSRVIndex();
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = isDepth ? DXGI_FORMAT_R32_FLOAT : resourceDesc.Format; // DXGI_FORMAT_D32_FLOAT can't be SRV
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, DirectX12DescriptorManager::Get()->GetSRVCPUHandle(m_DescriptorIndex));
    }

} // namespace VECTOR
