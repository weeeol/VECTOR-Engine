#pragma once

#include "Engine/Graphics/GraphicsContext.hpp"
#include <SDL.h>

namespace VECTOR {

    class OpenGLContext : public GraphicsContext {
    public:
        OpenGLContext(SDL_Window* windowHandle);
        ~OpenGLContext() override;

        void Init() override;
        void SwapBuffers() override;

    private:
        SDL_Window* m_WindowHandle;
        SDL_GLContext m_GLContext;
    };

} // namespace VECTOR
