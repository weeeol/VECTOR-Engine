#include "Engine/Core/SDLWindow.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/OpenGL/OpenGLContext.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Input/InputManager.hpp"

namespace VECTOR {

    static bool s_SDLInitialized = false;


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

    void SDLWindow::ProcessEvents(class InputManager* inputManager) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                m_ShouldClose = true;
            } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                if (inputManager) {
                    // Quick and dirty mapping for now
                    KeyCode key = (KeyCode)event.key.keysym.sym;
                    
                    // Remap some known ones if needed since SDL syms don't 1:1 match our KeyCode directly 
                    // if we used arbitrary values, but our KeyCode values match ASCII mostly.
                    // For special keys:
                    if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) key = KeyCode::Space;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) key = KeyCode::Escape;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) key = KeyCode::Enter;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_TAB) key = KeyCode::Tab;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) key = KeyCode::Right;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) key = KeyCode::Left;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) key = KeyCode::Down;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_UP) key = KeyCode::Up;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_F1) key = KeyCode::F1;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_F2) key = KeyCode::F2;
                    else if (event.key.keysym.scancode == SDL_SCANCODE_F3) key = KeyCode::F3;
                    
                    inputManager->SetKeyState(key, event.type == SDL_KEYDOWN);
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
                if (inputManager) {
                    MouseButton btn = MouseButton::Left;
                    if (event.button.button == SDL_BUTTON_RIGHT) btn = MouseButton::Right;
                    else if (event.button.button == SDL_BUTTON_MIDDLE) btn = MouseButton::Middle;
                    
                    inputManager->SetMouseButtonState(btn, event.type == SDL_MOUSEBUTTONDOWN);
                }
            }
        }
        
        // Update mouse position independently to get the latest state
        if (inputManager) {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            inputManager->SetMousePosition(mx, my);
            
            int dx, dy;
            SDL_GetRelativeMouseState(&dx, &dy);
            inputManager->SetMouseDelta(dx, dy);
        }
    }

} // namespace VECTOR
