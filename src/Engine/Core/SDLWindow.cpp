#include "Engine/Core/SDLWindow.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/OpenGL/OpenGLContext.hpp"
#include "Engine/Graphics/RendererAPI.hpp"

namespace VECTOR {

    static bool s_SDLInitialized = false;

    Window* Window::Create(const WindowProps& props) {
        return new SDLWindow(props);
    }

    SDLWindow::SDLWindow(const WindowProps& props) {
        Init(props);
    }

    SDLWindow::~SDLWindow() {
        Shutdown();
    }

    void SDLWindow::Init(const WindowProps& props) {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        VECTOR_LOG_INFO("Creating window " + props.Title + " (" + std::to_string(props.Width) + ", " + std::to_string(props.Height) + ")");

        if (!s_SDLInitialized) {
            int success = SDL_Init(SDL_INIT_VIDEO);
            if (success != 0) {
                VECTOR_LOG_ERROR("Could not initialize SDL!");
                return;
            }
            s_SDLInitialized = true;
        }

        uint32_t flags = SDL_WINDOW_SHOWN;
        if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) {
            flags |= SDL_WINDOW_OPENGL;
        }

        m_Window = SDL_CreateWindow(m_Data.Title.c_str(),
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    (int)m_Data.Width,
                                    (int)m_Data.Height,
                                    flags);

        if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) {
            m_Context = std::make_unique<OpenGLContext>(m_Window);
            m_Context->Init();
        }

        SetVSync(true);
    }

    void SDLWindow::Shutdown() {
        SDL_DestroyWindow(m_Window);
    }

    void SDLWindow::OnUpdate() {
        if (m_Context) {
            m_Context->SwapBuffers();
        }
    }

    void SDLWindow::SetVSync(bool enabled) {
        if (enabled) {
            SDL_GL_SetSwapInterval(1);
        } else {
            SDL_GL_SetSwapInterval(0);
        }
        m_Data.VSync = enabled;
    }

    bool SDLWindow::IsVSync() const {
        return m_Data.VSync;
    }

} // namespace VECTOR
