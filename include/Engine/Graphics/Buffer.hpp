#pragma once

#include <stdint.h>

namespace VECTOR {

    enum class BufferLayoutType {
        Mesh3D, // Pos(3), Normal(3), TexCoord(2)
        Quad2D  // Pos(2), TexCoord(2)
    };

    class VertexBuffer {
    public:
        virtual ~VertexBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void SetLayoutType(BufferLayoutType type) = 0;
        virtual BufferLayoutType GetLayoutType() const = 0;

        // Factory method
        static VertexBuffer* Create(const void* vertices, uint32_t size);
    };

    class IndexBuffer {
    public:
        virtual ~IndexBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual uint32_t GetCount() const = 0;

        // Factory method
        static IndexBuffer* Create(const uint32_t* indices, uint32_t count);
    };

} // namespace VECTOR
