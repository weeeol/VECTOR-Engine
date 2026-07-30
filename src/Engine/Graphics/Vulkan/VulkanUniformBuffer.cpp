#include "Engine/Graphics/Vulkan/VulkanUniformBuffer.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t bindingPoint)
        : m_Size(size), m_BindingPoint(bindingPoint)
    {
        // VMA_MEMORY_USAGE_CPU_TO_GPU guarantees host visibility and host coherence usually,
        // or at least mapped memory that we can write to directly.
        m_Buffer = std::make_unique<VulkanBuffer>(
            size, 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );
    }

    VulkanUniformBuffer::~VulkanUniformBuffer() {
    }

    void VulkanUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
        // Since VMA_MEMORY_USAGE_CPU_TO_GPU is used, UploadData maps, copies, and unmaps.
        // If an offset is needed, we'd need a more advanced Map implementation, but right now
        // the engine only calls SetData with the full data and offset 0.
        if (offset != 0) {
            return;
        }
        m_Buffer->UploadData(data, size);
    }

    void VulkanUniformBuffer::Bind() const {
        // In Vulkan, binding is handled via Descriptor Sets in SubmitMesh,
        // so this method is a no-op just like VulkanMesh::Draw().
    }

    void VulkanUniformBuffer::Unbind() const {
    }

} // namespace VECTOR
