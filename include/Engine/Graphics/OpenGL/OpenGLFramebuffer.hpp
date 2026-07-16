#pragma once

#include "Engine/Graphics/Framebuffer.hpp"

namespace VECTOR {

    class OpenGLFramebuffer : public Framebuffer {
    public:
        OpenGLFramebuffer(const FramebufferSpecification& spec);
        virtual ~OpenGLFramebuffer();

        void Invalidate();

        virtual void Bind() override;
        virtual void Unbind() override;

        virtual void Resize(uint32_t width, uint32_t height) override;

        virtual unsigned int GetColorAttachmentRendererID() const override { return m_ColorAttachment; }
        virtual unsigned int GetDepthAttachmentRendererID() const override { return m_DepthAttachment; }

        virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_ColorAttachment = 0;
        uint32_t m_DepthAttachment = 0;
        FramebufferSpecification m_Specification;
    };

} // namespace VECTOR
