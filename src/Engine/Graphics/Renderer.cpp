#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/Texture.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

namespace VECTOR {

    Renderer::Renderer() : m_Window(nullptr), m_Renderer(nullptr) {}

    Renderer::~Renderer() {
        Shutdown();
    }

    bool Renderer::Initialize(const std::string& title, int width, int height) {
        m_Window = SDL_CreateWindow(
            title.c_str(),
            width,
            height,
            0 // Flags
        );

        if (!m_Window) {
            VECTOR_LOG_ERROR(std::string("Failed to create window! SDL_Error: ") + SDL_GetError());
            SDL_assert(false && "Failed to create SDL_Window");
            return false;
        }

        m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
        SDL_SetRenderVSync(m_Renderer, 1);

        if (!m_Renderer) {
            VECTOR_LOG_ERROR(std::string("Failed to create renderer! SDL_Error: ") + SDL_GetError());
            SDL_assert(false && "Failed to create SDL_Renderer");
            return false;
        }

        if (TTF_Init() == false) {
            VECTOR_LOG_ERROR(std::string("Failed to initialize SDL_ttf! TTF_Error: ") + SDL_GetError());
            SDL_assert(false && "Failed to initialize SDL_ttf");
            return false;
        }

        VECTOR_LOG_INFO("Renderer initialized successfully.");
        return true;
    }

    void Renderer::Shutdown() {

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
        SDL_FRect rect = { (float)x, (float)y, (float)w, (float)h };
        SDL_SetRenderDrawBlendMode(m_Renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(m_Renderer, r, g, b, a);
        SDL_RenderFillRect(m_Renderer, &rect);
        SDL_SetRenderDrawBlendMode(m_Renderer, SDL_BLENDMODE_NONE); // Reset
    }

    void Renderer::DrawText(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b, int fontSize) {
        TTF_Font* font = ResourceManager::Get().GetFont("assets/font.ttf", fontSize);
        if (!font) return;

        SDL_Color color = { r, g, b, 255 };
        // Use Blended instead of Solid for high-quality, anti-aliased text!
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);

        SDL_FRect destRect = { (float)x, (float)y, (float)surface->w, (float)surface->h };
        SDL_RenderTexture(m_Renderer, texture, nullptr, &destRect);

        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }

    void Renderer::DrawTexture(Texture* texture, int x, int y, int w, int h) {
        if (!texture || !texture->IsValid()) return;
        
        SDL_FRect destRect;
        destRect.x = (float)x;
        destRect.y = (float)y;
        destRect.w = (float)((w == -1) ? texture->GetWidth() : w);
        destRect.h = (float)((h == -1) ? texture->GetHeight() : h);

        SDL_RenderTexture(m_Renderer, texture->GetSDLTexture(), nullptr, &destRect);
    }

    void Renderer::SetRenderTarget(Texture* texture) {
        if (texture && texture->IsValid()) {
            SDL_SetRenderTarget(m_Renderer, texture->GetSDLTexture());
        }
    }

    void Renderer::ResetRenderTarget() {
        SDL_SetRenderTarget(m_Renderer, nullptr);
    }

    void Renderer::SetRenderDrawBlendMode(SDL_BlendMode mode) {
        SDL_SetRenderDrawBlendMode(m_Renderer, mode);
    }

    void Renderer::SetBorderless(bool borderless) {
        if (!m_Window) return;
        if (borderless) {
            SDL_SetWindowBordered(m_Window, false);
            // Get display bounds to fill the screen
            SDL_DisplayID displayId = SDL_GetDisplayForWindow(m_Window);
            if (displayId) {
                const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayId);
                if (mode) {
                    SDL_SetWindowSize(m_Window, mode->w, mode->h);
                    SDL_SetWindowPosition(m_Window, 0, 0);
                }
            }
        } else {
            SDL_SetWindowBordered(m_Window, true);
            // Restore to a reasonable centered position
            int w, h;
            SDL_GetWindowSize(m_Window, &w, &h);
            SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
        VECTOR_LOG_INFO(std::string("Window mode set to: ") + (borderless ? "Borderless" : "Windowed"));
    }

    void Renderer::SetResolution(int width, int height) {
        if (!m_Window) return;
        SDL_SetWindowSize(m_Window, width, height);
        SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        VECTOR_LOG_INFO("Resolution set to: " + std::to_string(width) + "x" + std::to_string(height));
    }

} // namespace VECTOR
