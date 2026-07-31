#pragma once

#include "Engine/Graphics/UniformBufferObject.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Buffer.hpp"
#include <memory>

namespace VECTOR {

    class DirectX12UniformBuffer : public UniformBuffer {
    public:
        DirectX12UniformBuffer(uint32_t size, uint32_t binding);
        virtual ~DirectX12UniformBuffer();

        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
        virtual uint32_t GetBindingPoint() const override { return m_Binding; }
        virtual void Bind() const override {}
        virtual void Unbind() const override {}

        // DX12 specific methods
        ID3D12Resource* GetResource() const { return m_Buffer->GetResource(); }
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_Buffer->GetResource()->GetGPUVirtualAddress(); }
        uint32_t GetSize() const { return m_Size; }

    private:
        std::unique_ptr<DirectX12Buffer> m_Buffer;
        uint32_t m_Size = 0;
        uint32_t m_Binding = 0;
        void* m_MappedData = nullptr;
    };

} // namespace VECTOR
