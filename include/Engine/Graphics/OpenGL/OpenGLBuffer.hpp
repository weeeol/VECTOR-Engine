#pragma once

#include "Engine/Graphics/Buffer.hpp"

namespace VECTOR {

    class OpenGLVertexBuffer : public VertexBuffer {
    public:
        OpenGLVertexBuffer(const void* vertices, uint32_t size);
        virtual ~OpenGLVertexBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void SetLayoutType(BufferLayoutType type) override { m_LayoutType = type; }
        virtual BufferLayoutType GetLayoutType() const override { return m_LayoutType; }

    private:
        uint32_t m_RendererID;
        BufferLayoutType m_LayoutType = BufferLayoutType::Mesh3D;
    };

    class OpenGLIndexBuffer : public IndexBuffer {
    public:
        OpenGLIndexBuffer(const uint32_t* indices, uint32_t count);
        virtual ~OpenGLIndexBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual uint32_t GetCount() const override { return m_Count; }

    private:
        uint32_t m_RendererID;
        uint32_t m_Count;
    };

} // namespace VECTOR
