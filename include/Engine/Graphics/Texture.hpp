#pragma once

#include <SDL3/SDL.h>
#include <string>

namespace VECTOR {

    class Renderer; // Forward declaration

    class Texture {
    public:
        Texture(Renderer* renderer, const std::string& filepath);
        // Constructor for creating an empty render target texture
        Texture(Renderer* renderer, int width, int height);
        ~Texture();

        // Prevent copying
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        SDL_Texture* GetSDLTexture() const { return m_Texture; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }
        bool IsValid() const { return m_Texture != nullptr; }

    private:
        SDL_Texture* m_Texture;
        int m_Width;
        int m_Height;
    };

} // namespace VECTOR
