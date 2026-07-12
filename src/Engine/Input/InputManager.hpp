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
        void Update();

        // Check if a specific key is currently held down
        // SDL_Scancode is used (e.g., SDL_SCANCODE_W, SDL_SCANCODE_UP)
        bool IsKeyPressed(SDL_Scancode key) const;

    private:
        const Uint8* m_KeyboardState;
    };

} // namespace VECTOR
