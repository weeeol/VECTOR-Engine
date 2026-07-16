#include "Engine/Input/InputManager.hpp"

namespace VECTOR {

    InputManager::InputManager() 
        : m_MouseX(0), m_MouseY(0), m_MouseDeltaX(0), m_MouseDeltaY(0) {
    }

    InputManager::~InputManager() {
    }

    void InputManager::Update() {
        m_PrevMouseButtons = m_MouseButtons;
        // Mouse Delta is only valid for the frame it was received in
        m_MouseDeltaX = 0;
        m_MouseDeltaY = 0;
    }

    void InputManager::SetRelativeMouseMode(bool enable) {
        // This still requires an external platform call, which will be handled later
        // via a Window or Platform abstraction call.
    }

    bool InputManager::IsKeyPressed(KeyCode key) const {
        auto it = m_Keys.find(key);
        if (it != m_Keys.end()) {
            return it->second;
        }
        return false;
    }

    bool InputManager::IsMouseButtonPressed(MouseButton button) const {
        auto it = m_MouseButtons.find(button);
        if (it != m_MouseButtons.end()) {
            return it->second;
        }
        return false;
    }
    
    bool InputManager::IsMouseButtonJustPressed(MouseButton button) const {
        bool isPressed = IsMouseButtonPressed(button);
        
        bool wasPressed = false;
        auto it = m_PrevMouseButtons.find(button);
        if (it != m_PrevMouseButtons.end()) {
            wasPressed = it->second;
        }
        
        return isPressed && !wasPressed;
    }

    void InputManager::SetKeyState(KeyCode key, bool isPressed) {
        m_Keys[key] = isPressed;
    }

    void InputManager::SetMouseButtonState(MouseButton button, bool isPressed) {
        m_MouseButtons[button] = isPressed;
    }

    void InputManager::SetMousePosition(int x, int y) {
        m_MouseX = x;
        m_MouseY = y;
    }

    void InputManager::SetMouseDelta(int dx, int dy) {
        m_MouseDeltaX = dx;
        m_MouseDeltaY = dy;
    }

} // namespace VECTOR
