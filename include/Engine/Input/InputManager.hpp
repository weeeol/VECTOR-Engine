#pragma once

#include <SDL3/SDL.h>
#include <unordered_map>

namespace VECTOR {

    /**
     * @class InputManager
     * @brief Manages keyboard input state.
     */
    class InputManager {
    public:
        InputManager();
        ~InputManager();

        // Update keyboard state at the beginning of the frame
        void Update();

        // Check if a specific key is currently held down
        // SDL_Scancode is used (e.g., SDL_SCANCODE_W, SDL_SCANCODE_UP)
        bool IsKeyPressed(SDL_Scancode key) const;

        // Mouse input
        float GetMouseX() const { return m_MouseX; }
        float GetMouseY() const { return m_MouseY; }
        float GetMouseDeltaX() const { return m_MouseDeltaX; }
        float GetMouseDeltaY() const { return m_MouseDeltaY; }
        
        void SetRelativeMouseMode(bool enable);
        bool IsMouseButtonPressed(int button) const; // Use SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT, etc.
        
        // Check if mouse button was just pressed this frame
        bool IsMouseButtonJustPressed(int button) const;
        
        void SetIgnoreImGuiCapture(bool ignore) { m_IgnoreImGuiCapture = ignore; }
        bool GetIgnoreImGuiCapture() const { return m_IgnoreImGuiCapture; }

    private:
        const bool* m_KeyboardState;
        uint32_t m_MouseState;
        uint32_t m_PrevMouseState;
        
        float m_MouseX, m_MouseY;
        float m_MouseDeltaX, m_MouseDeltaY;
        
        bool m_IgnoreImGuiCapture = false;
    };

} // namespace VECTOR
