#include "OpenGLTexture2D.hpp"
#include "Engine/Core/Logger.hpp"
#include <GL/glew.h>
#include <SDL_image.h>

namespace VECTOR {

    OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
        : m_RendererID(0), m_FilePath(path), m_Width(0), m_Height(0), m_BPP(0)
    {
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            VECTOR_LOG_ERROR("Failed to load texture: " + path + " - " + IMG_GetError());
            return;
        }

        m_Width = surface->w;
        m_Height = surface->h;

        int mode = GL_RGB;
        int format = GL_RGB;

        if (surface->format->BytesPerPixel == 4) {
            mode = GL_RGBA;
            if (surface->format->Rmask == 0x00ff0000) {
                format = GL_BGRA;
            } else {
                format = GL_RGBA;
            }
        } else if (surface->format->BytesPerPixel == 3) {
            mode = GL_RGB;
            if (surface->format->Rmask == 0x00ff0000) {
                format = GL_BGR;
            } else {
                format = GL_RGB;
            }
        }

        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch / surface->format->BytesPerPixel);

        glTexImage2D(GL_TEXTURE_2D, 0, mode, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
        
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);
        SDL_FreeSurface(surface);
    }

    OpenGLTexture2D::~OpenGLTexture2D() {
        if (m_RendererID) {
            glDeleteTextures(1, &m_RendererID);
        }
    }

    void OpenGLTexture2D::Bind(unsigned int slot) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
    }

    void OpenGLTexture2D::Unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

} // namespace VECTOR
