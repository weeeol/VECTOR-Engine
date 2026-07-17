#pragma once

#include "Engine/Core/Scene.hpp"
#include "Engine/Graphics/ParticleSystem.hpp"
#include <memory>

namespace VECTOR {
    class InputManager;
}

namespace Game {

    class SplashScene : public VECTOR::Scene {
    public:
        SplashScene(int width, int height, VECTOR::InputManager* inputManager);
        ~SplashScene() override = default;

        void OnEnter() override;
        void Update(float deltaTime) override;
        void Render(VECTOR::Renderer* renderer) override;

    private:
        void TransitionToMenu();

        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;
        VECTOR::ParticleEmitter m_ParticleEmitter;

        float m_Timer = 0.0f;
        int m_Phase = 0;         // 0 = Engine, 1 = Created by, 2 = Game title
        bool m_Skipping = false;
    };

} // namespace Game
