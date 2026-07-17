#pragma once
#include <SDL3/SDL.h>

namespace Game {

    struct PlayerInputComponent {
        SDL_Scancode upKey;
        SDL_Scancode downKey;
    };

    enum class AIDifficulty {
        Easy,
        Medium,
        Hard
    };

    enum class AIState {
        Idle,
        Tracking,
        Predicting
    };

    struct AIComponent {
        AIDifficulty difficulty;
        AIState state;
        float reactionDelayTimer;
        float reactionTime;
        float targetY;
    };

    struct BallComponent {
        bool isActive;
    };

} // namespace Game
