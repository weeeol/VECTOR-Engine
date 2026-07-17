#include "Engine/Graphics/Texture.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include <SDL3_image/SDL_image.h>
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    Texture::Texture(Renderer* renderer, const std::string& filepath)
        : m_Texture(nullptr), m_Width(0), m_Height(0) 
    {
        SDL_Surface* surface = IMG_Load(filepath.c_str());
        if (!surface) {
            VECTOR_LOG_ERROR(std::string("Failed to load image: ") + filepath + ". IMG_Error: " + SDL_GetError());
            return;
        }

        // Enable transparency for pure black (0, 0, 0)
        SDL_SetSurfaceColorKey(surface, true, SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format), nullptr, 0, 0, 0));

        m_Texture = SDL_CreateTextureFromSurface(renderer->GetSDLRenderer(), surface);
        if (!m_Texture) {
            VECTOR_LOG_ERROR(std::string("Failed to create texture from surface. SDL_Error: ") + SDL_GetError());
        } else {
            m_Width = surface->w;
            m_Height = surface->h;
        }

        SDL_DestroySurface(surface);
    }

    Texture::Texture(Renderer* renderer, int width, int height)
        : m_Texture(nullptr), m_Width(width), m_Height(height)
    {
        m_Texture = SDL_CreateTexture(
            renderer->GetSDLRenderer(),
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            width, height
        );

        if (!m_Texture) {
            VECTOR_LOG_ERROR(std::string("Failed to create target texture. SDL_Error: ") + SDL_GetError());
        }
    }

    Texture::~Texture() {
        if (m_Texture) {
            SDL_DestroyTexture(m_Texture);
            m_Texture = nullptr;
        }
    }

} // namespace VECTOR
