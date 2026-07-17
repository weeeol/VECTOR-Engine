#include "Engine/Input/InputManager.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include <imgui.h>
namespace VECTOR {

    InputManager::InputManager() : m_MouseState(0), m_PrevMouseState(0), m_MouseX(0.0f), m_MouseY(0.0f), m_MouseDeltaX(0.0f), m_MouseDeltaY(0.0f) {
        // Initialize keyboard state pointer
        int numkeys;
        m_KeyboardState = SDL_GetKeyboardState(&numkeys);
    }

    InputManager::~InputManager() {
    }

    void InputManager::Update() {
        m_PrevMouseState = m_MouseState;
        m_MouseState = SDL_GetMouseState(&m_MouseX, &m_MouseY);
        SDL_GetRelativeMouseState(&m_MouseDeltaX, &m_MouseDeltaY);

        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
            m_MouseDeltaX = 0.0f;
            m_MouseDeltaY = 0.0f;
        }
    }

    void InputManager::SetRelativeMouseMode(bool enable) {
        SDL_Window* window = nullptr;
        if (Application::Get().GetRenderer()) {
            window = Application::Get().GetRenderer()->GetWindow();
        }
        if (window) {
            SDL_SetWindowRelativeMouseMode(window, enable);
        }
    }

    bool InputManager::IsKeyPressed(SDL_Scancode key) const {
        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard) {
            return false;
        }
        if (m_KeyboardState) {
            return m_KeyboardState[key] == 1;
        }
        return false;
    }

    bool InputManager::IsMouseButtonPressed(int button) const {
        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
            return false;
        }
        return (m_MouseState & SDL_BUTTON_MASK(button)) != 0;
    }
    
    bool InputManager::IsMouseButtonJustPressed(int button) const {
        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
            return false;
        }
        bool isPressedNow = (m_MouseState & SDL_BUTTON_MASK(button)) != 0;
        bool wasPressedBefore = (m_PrevMouseState & SDL_BUTTON_MASK(button)) != 0;
        return isPressedNow && !wasPressedBefore;
    }

} // namespace VECTOR
