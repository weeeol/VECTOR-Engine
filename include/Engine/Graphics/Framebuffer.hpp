#pragma once

#include <memory>
#include <vector>
#include <cstdint>

namespace VECTOR {

    struct FramebufferSpecification {
        uint32_t Width, Height;
        bool DepthOnly = false;
        // In a full implementation, you'd have attachment formats here.
    };

    class Framebuffer {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual unsigned int GetColorAttachmentRendererID() const = 0;
        virtual unsigned int GetDepthAttachmentRendererID() const = 0;

        virtual const FramebufferSpecification& GetSpecification() const = 0;

        static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);
    };

} // namespace VECTOR
