#include "Texture.hpp"
#include "Renderer.hpp"
#include <SDL_image.h>
#include <iostream>

namespace VECTOR {

    Texture::Texture(Renderer* renderer, const std::string& filepath)
        : m_Texture(nullptr), m_Width(0), m_Height(0) 
    {
        SDL_Surface* surface = IMG_Load(filepath.c_str());
        if (!surface) {
            std::cerr << "Failed to load image: " << filepath << ". IMG_Error: " << IMG_GetError() << std::endl;
            return;
        }

        m_Texture = SDL_CreateTextureFromSurface(renderer->GetSDLRenderer(), surface);
        if (!m_Texture) {
            std::cerr << "Failed to create texture from surface. SDL_Error: " << SDL_GetError() << std::endl;
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
