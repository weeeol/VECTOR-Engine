#include "Engine/Graphics/Texture.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include <SDL_image.h>
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    Texture::Texture(Renderer* renderer, const std::string& filepath)
        : m_Texture(nullptr), m_Width(0), m_Height(0) 
    {
        SDL_Surface* surface = IMG_Load(filepath.c_str());
        if (!surface) {
            VECTOR_LOG_ERROR(std::string("Failed to load image: ") + filepath + ". IMG_Error: " + IMG_GetError());
            return;
        }

        // Enable transparency for pure black (0, 0, 0)
        SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0, 0, 0));

        m_Texture = SDL_CreateTextureFromSurface(renderer->GetSDLRenderer(), surface);
        if (!m_Texture) {
            VECTOR_LOG_ERROR(std::string("Failed to create texture from surface. SDL_Error: ") + SDL_GetError());
        } else {
            m_Width = surface->w;
            m_Height = surface->h;
        }

        SDL_FreeSurface(surface);
    }

    Texture::~Texture() {
        if (m_Texture) {
            SDL_DestroyTexture(m_Texture);
            m_Texture = nullptr;
        }
    }

} // namespace VECTOR
