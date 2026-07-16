#include "Engine/Graphics/DirectX/DirectX12RendererAPI.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    void DirectX12RendererAPI::Init() {
        VECTOR_LOG_INFO("DirectX12RendererAPI::Init (Stub)");
    }

    void DirectX12RendererAPI::SetClearColor(float r, float g, float b, float a) {
        m_ClearColor[0] = r;
        m_ClearColor[1] = g;
        m_ClearColor[2] = b;
        m_ClearColor[3] = a;
    }

    void DirectX12RendererAPI::Clear() {
        // Since we don't have global state, this depends on the Context having a CommandList we can use
        // A complete implementation would get the CommandList from the Context, transition the RTV, clear it, etc.
        // For now, it's stubbed as we need a way for the Renderer to submit commands properly.
    }

} // namespace VECTOR
