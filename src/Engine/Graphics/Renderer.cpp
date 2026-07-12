#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/Texture.hpp"
#include "Engine/Core/Logger.hpp"
#include <SDL_ttf.h>
#include <SDL_image.h>

namespace VECTOR {

    Renderer::Renderer() : m_Window(nullptr), m_Renderer(nullptr) {}

    Renderer::~Renderer() {
        Shutdown();
    }

    bool Renderer::Initialize(const std::string& title, int width, int height) {
        m_Window = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            0 // Flags
        );

        if (!m_Window) {
            VECTOR_LOG_ERROR(std::string("Failed to create window! SDL_Error: ") + SDL_GetError());
            SDL_assert(false && "Failed to create SDL_Window");
            return false;
        }

        m_Renderer = SDL_CreateRenderer(
            m_Window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );

        if (!m_Renderer) {
            VECTOR_LOG_ERROR(std::string("Failed to create renderer! SDL_Error: ") + SDL_GetError());
            SDL_assert(false && "Failed to create SDL_Renderer");
            return false;
        }

        if (TTF_Init() == -1) {
            VECTOR_LOG_ERROR(std::string("Failed to initialize SDL_ttf! TTF_Error: ") + TTF_GetError());
            SDL_assert(false && "Failed to initialize SDL_ttf");
            return false;
        }

        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            VECTOR_LOG_ERROR(std::string("Failed to initialize SDL_image! IMG_Error: ") + IMG_GetError());
            SDL_assert(false && "Failed to initialize SDL_image");
            return false;
        }

        VECTOR_LOG_INFO("Renderer initialized successfully.");
        return true;
    }

    void Renderer::Shutdown() {
        for (auto& pair : m_Fonts) {
            if (pair.second) TTF_CloseFont(pair.second);
        }
        m_Fonts.clear();

        IMG_Quit();
        TTF_Quit();
        if (m_Renderer) {
            SDL_DestroyRenderer(m_Renderer);
            m_Renderer = nullptr;
        }
        if (m_Window) {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    void Renderer::Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        SDL_SetRenderDrawColor(m_Renderer, r, g, b, a);
        SDL_RenderClear(m_Renderer);
    }

    void Renderer::Present() {
        SDL_RenderPresent(m_Renderer);
    }

    void Renderer::DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        SDL_Rect rect = { x, y, w, h };
        SDL_SetRenderDrawColor(m_Renderer, r, g, b, a);
        SDL_RenderFillRect(m_Renderer, &rect);
    }

    void Renderer::DrawText(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b, int fontSize) {
        if (m_Fonts.find(fontSize) == m_Fonts.end()) {
            TTF_Font* font = TTF_OpenFont("assets/font.ttf", fontSize);
            if (!font) {
                VECTOR_LOG_WARN(std::string("Failed to load font size ") + std::to_string(fontSize) + "! TTF_Error: " + TTF_GetError());
                return;
            }
            m_Fonts[fontSize] = font;
        }

        TTF_Font* font = m_Fonts[fontSize];

        SDL_Color color = { r, g, b, 255 };
        // Use Blended instead of Solid for high-quality, anti-aliased text!
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);

        SDL_Rect destRect = { x, y, surface->w, surface->h };
        SDL_RenderCopy(m_Renderer, texture, nullptr, &destRect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    void Renderer::DrawTexture(Texture* texture, int x, int y, int w, int h) {
        if (!texture || !texture->IsValid()) return;
        
        SDL_Rect destRect;
        destRect.x = x;
        destRect.y = y;
        destRect.w = (w == -1) ? texture->GetWidth() : w;
        destRect.h = (h == -1) ? texture->GetHeight() : h;

        SDL_RenderCopy(m_Renderer, texture->GetSDLTexture(), nullptr, &destRect);
    }

} // namespace VECTOR
