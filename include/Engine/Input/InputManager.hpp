#pragma once

#include <SDL.h>
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
        void PrepareForEvents();
        void ProcessEvent(const SDL_Event& e);
        void Update();
        void ClearJustPressed();

        // Check if a specific key is currently held down
        // SDL_Scancode is used (e.g., SDL_SCANCODE_W, SDL_SCANCODE_UP)
        bool IsKeyPressed(SDL_Scancode key) const;

        // Mouse input
        int GetMouseX() const { return m_MouseX; }
        int GetMouseY() const { return m_MouseY; }
        bool IsMouseButtonPressed(int button) const; // Use SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT, etc.
        
        // Check if mouse button was just pressed this frame
        bool IsMouseButtonJustPressed(int button) const;

    private:
        const Uint8* m_KeyboardState;
        Uint32 m_MouseState;
        int m_MouseX;
        int m_MouseY;
        bool m_MouseJustPressed[6];
    };

} // namespace VECTOR
