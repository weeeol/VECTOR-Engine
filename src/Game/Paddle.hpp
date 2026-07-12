#pragma once

#include "Engine/GameObject.hpp"
#include "Engine/Renderer.hpp"
#include "Engine/InputManager.hpp"
#include <SDL.h>

namespace Game {

    class Paddle : public VECTOR::GameObject {
    public:
        Paddle(float x, float y, SDL_Scancode upKey, SDL_Scancode downKey);

        void Update(float deltaTime, const VECTOR::InputManager* inputManager, int screenHeight);
        void Render(VECTOR::Renderer* renderer) const override;

        VECTOR::AABB GetAABB() const;
        void Reset(float x, float y);

    protected:
        float m_Speed;
        float m_Width;
        float m_Height;
        
        SDL_Scancode m_UpKey;
        SDL_Scancode m_DownKey;
    };

} // namespace Game
