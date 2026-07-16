#include "Engine/Core/Window.hpp"
#include "Engine/Core/SDLWindow.hpp"
#include "Engine/Core/Win32Window.hpp"
#include "Engine/Graphics/RendererAPI.hpp"

namespace VECTOR {

    Window* Window::Create(const WindowProps& props) {
        if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12) {
            return new Win32Window(props);
        } else {
            return new SDLWindow(props);
        }
    }

} // namespace VECTOR
