#include "Engine/Graphics/Framebuffer.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLFramebuffer.hpp"
#include "Engine/Graphics/DirectX/DirectX12Framebuffer.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec) {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLFramebuffer>(spec);
            case RendererAPI::API::DirectX12:
                return std::make_shared<DirectX12Framebuffer>(spec);
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
