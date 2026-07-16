#pragma once

#include "Engine/Input/KeyCodes.hpp"
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
        bool IsKeyPressed(KeyCode key) const;

        // Mouse input
        int GetMouseX() const { return m_MouseX; }
        int GetMouseY() const { return m_MouseY; }
        int GetMouseDeltaX() const { return m_MouseDeltaX; }
        int GetMouseDeltaY() const { return m_MouseDeltaY; }
        
        void SetRelativeMouseMode(bool enable);
        bool IsMouseButtonPressed(MouseButton button) const;
        
        // Check if mouse button was just pressed this frame
        bool IsMouseButtonJustPressed(MouseButton button) const;

        // Called by Window ProcessEvents to update state
        void SetKeyState(KeyCode key, bool isPressed);
        void SetMouseButtonState(MouseButton button, bool isPressed);
        void SetMousePosition(int x, int y);
        void SetMouseDelta(int dx, int dy);

    private:
        std::unordered_map<KeyCode, bool> m_Keys;
        std::unordered_map<MouseButton, bool> m_MouseButtons;
        std::unordered_map<MouseButton, bool> m_PrevMouseButtons;
        int m_MouseX;
        int m_MouseY;
        int m_MouseDeltaX;
        int m_MouseDeltaY;
    };

} // namespace VECTOR
