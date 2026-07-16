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
        // Will clear the Render Target View using m_ClearColor
    }

} // namespace VECTOR
