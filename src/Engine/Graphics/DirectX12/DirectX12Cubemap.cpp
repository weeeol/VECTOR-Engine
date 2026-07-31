#include "Engine/Graphics/DirectX12/DirectX12Cubemap.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX12/DirectX12DescriptorManager.hpp"
#include "Engine/Core/Logger.hpp"
#include "stb/stb_image.h"

namespace VECTOR {

    DirectX12Cubemap::DirectX12Cubemap(const std::vector<std::string>& faces) {
        LoadFaces(faces);
    }

    DirectX12Cubemap::~DirectX12Cubemap() {
        if (m_DescriptorIndex != -1) {
            DirectX12DescriptorManager::Get()->FreeSRVIndex(m_DescriptorIndex);
        }
    }

    void DirectX12Cubemap::LoadFaces(const std::vector<std::string>& facePaths) {
        if (facePaths.size() != 6) {
            VECTOR_LOG_ERROR("Cubemap requires exactly 6 face textures.");
            return;
        }

        stbi_set_flip_vertically_on_load(false);

        int width, height, channels;
        std::vector<float*> faceData(6);
        for (int i = 0; i < 6; i++) {
            faceData[i] = stbi_loadf(facePaths[i].c_str(), &width, &height, &channels, 4);
            if (!faceData[i]) {
                VECTOR_LOG_ERROR("Failed to load cubemap face");
                for (int j = 0; j < i; j++) stbi_image_free(faceData[j]);
                return;
            }
        }

        auto allocator = DirectX12Context::Get()->GetAllocator();
        auto device = DirectX12Context::Get()->GetDevice();

        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = width;
        resourceDesc.Height = height;
        resourceDesc.DepthOrArraySize = 6;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
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
            VECTOR_LOG_ERROR("Failed to create Cubemap resource.");
            for (int i = 0; i < 6; i++) stbi_image_free(faceData[i]);
            return;
        }

        m_DescriptorIndex = DirectX12DescriptorManager::Get()->AllocateSRVIndex();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = resourceDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = 1;
        device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, DirectX12DescriptorManager::Get()->GetSRVCPUHandle(m_DescriptorIndex));

        // Upload Data
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(6);
        std::vector<UINT> numRows(6);
        std::vector<UINT64> rowSizeInBytes(6);
        UINT64 totalBytes = 0;

        device->GetCopyableFootprints(&resourceDesc, 0, 6, 0, footprints.data(), numRows.data(), rowSizeInBytes.data(), &totalBytes);

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
            
            for (int i = 0; i < 6; i++) {
                uint8_t* pSrc = reinterpret_cast<uint8_t*>(faceData[i]);
                for (UINT y = 0; y < numRows[i]; ++y) {
                    memcpy(pData + footprints[i].Offset + y * footprints[i].Footprint.RowPitch,
                           pSrc + y * rowSizeInBytes[i], rowSizeInBytes[i]);
                }
            }
            uploadBuffer->Unmap(0, nullptr);

            DirectX12Context::Get()->ExecuteCommandListSync([&](ID3D12GraphicsCommandList* cmdList) {
                for (int i = 0; i < 6; i++) {
                    D3D12_TEXTURE_COPY_LOCATION dst = {};
                    dst.pResource = m_Resource.Get();
                    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    dst.SubresourceIndex = i;

                    D3D12_TEXTURE_COPY_LOCATION src = {};
                    src.pResource = uploadBuffer.Get();
                    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                    src.PlacedFootprint = footprints[i];
                    
                    cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
                }
                
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = m_Resource.Get();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                cmdList->ResourceBarrier(1, &barrier);
            });
        }

        for (int i = 0; i < 6; i++) stbi_image_free(faceData[i]);
        VECTOR_LOG_INFO("DirectX 12 Cubemap loaded successfully.");
    }

} // namespace VECTOR
