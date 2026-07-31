#include "Engine/Graphics/DirectX12/DirectX12UniformBuffer.hpp"
#include <cstring>

namespace VECTOR {

    DirectX12UniformBuffer::DirectX12UniformBuffer(uint32_t size, uint32_t binding)
        : m_Size(size), m_Binding(binding) {
        
        // DX12 constant buffers must be 256-byte aligned
        uint32_t alignedSize = (size + 255) & ~255;
        
        m_Buffer = std::make_unique<DirectX12Buffer>(
            alignedSize,
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_RESOURCE_STATE_GENERIC_READ
        );

        m_Buffer->Map(&m_MappedData);
    }

    DirectX12UniformBuffer::~DirectX12UniformBuffer() {
        if (m_Buffer) {
            m_Buffer->Unmap();
        }
    }

    void DirectX12UniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
        if (m_MappedData) {
            std::memcpy((uint8_t*)m_MappedData + offset, data, size);
        }
    }

} // namespace VECTOR
