#pragma once

#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include <unordered_map>

namespace VECTOR {

    /**
     * @class Renderer
     * @brief Wraps SDL_Window and SDL_Renderer.
     * 
     * Hides the SDL-specific graphics calls from the game logic.
     */
    class Renderer {
    public:
        Renderer();
        ~Renderer();

        // Prevent copying
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        // Initialize the window and renderer
        bool Initialize(const std::string& title, int width, int height);
        
        // Clean up resources
        void Shutdown();

        // Clear the screen with a specific color (RGBA 0-255)
        void Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        
        // Present the rendered buffer to the screen
        void Present();

        // Draw a filled rectangle
        void DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

        // Draw text
        void DrawText(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b, int fontSize = 24);

        // Draw texture
        void DrawTexture(class Texture* texture, int x, int y, int w = -1, int h = -1);

        SDL_Renderer* GetSDLRenderer() const { return m_Renderer; }

    private:
        SDL_Window* m_Window;
        SDL_Renderer* m_Renderer;
    };

} // namespace VECTOR
