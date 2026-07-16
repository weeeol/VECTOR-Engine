#include "Engine/Graphics/OpenGL/OpenGLContext.hpp"
#include "Engine/Core/Logger.hpp"
#include <GL/glew.h>
#include <SDL_opengl.h>

namespace VECTOR {

    OpenGLContext::OpenGLContext(SDL_Window* windowHandle)
        : m_WindowHandle(windowHandle), m_GLContext(nullptr) {
        if (!windowHandle) {
            VECTOR_LOG_ERROR("Window handle is null!");
        }
    }

    OpenGLContext::~OpenGLContext() {
        if (m_GLContext) {
            SDL_GL_DeleteContext(m_GLContext);
        }
    }

    void OpenGLContext::Init() {
        VECTOR_LOG_INFO("Creating OpenGL Context...");
        
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        m_GLContext = SDL_GL_CreateContext(m_WindowHandle);
        if (!m_GLContext) {
            VECTOR_LOG_ERROR("Failed to create OpenGL context.");
            return;
        }

        SDL_GL_MakeCurrent(m_WindowHandle, m_GLContext);

        VECTOR_LOG_INFO("Initializing GLEW...");
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            VECTOR_LOG_ERROR("Failed to initialize GLEW.");
            return;
        }

        VECTOR_LOG_INFO("OpenGL Info:");
        VECTOR_LOG_INFO("  Vendor: " + std::string((const char*)glGetString(GL_VENDOR)));
        VECTOR_LOG_INFO("  Renderer: " + std::string((const char*)glGetString(GL_RENDERER)));
        VECTOR_LOG_INFO("  Version: " + std::string((const char*)glGetString(GL_VERSION)));
    }

    void OpenGLContext::SwapBuffers() {
        SDL_GL_SwapWindow(m_WindowHandle);
    }

} // namespace VECTOR
