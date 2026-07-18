#pragma once

#include "Engine/Graphics/UniformBufferObject.hpp"
#include "Engine/Graphics/Vulkan/VulkanBuffer.hpp"
#include <memory>

namespace VECTOR {

    class VulkanUniformBuffer : public UniformBuffer {
    public:
        VulkanUniformBuffer(uint32_t size, uint32_t bindingPoint);
        ~VulkanUniformBuffer() override;

        void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
        void Bind() const override;
        void Unbind() const override;
        uint32_t GetBindingPoint() const override { return m_BindingPoint; }

        VkBuffer GetBuffer() const { return m_Buffer->GetBuffer(); }
        uint32_t GetSize() const { return m_Size; }

    private:
        std::unique_ptr<VulkanBuffer> m_Buffer;
        uint32_t m_BindingPoint;
        uint32_t m_Size;
    };

} // namespace VECTOR
