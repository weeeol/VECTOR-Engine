#pragma once

#include "Engine/ECS/System.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/ParticleSystem.hpp"
#include "Game/Core/GameComponents.hpp"

namespace Game {

    class PlayerInputSystem : public VECTOR::System {
    public:
        PlayerInputSystem(VECTOR::InputManager* inputManager, int screenHeight);
        void Update(VECTOR::Registry& registry, float deltaTime) override;
    private:
        VECTOR::InputManager* m_InputManager;
        int m_ScreenHeight;
    };

    class AISystem : public VECTOR::System {
    public:
        AISystem(VECTOR::Entity ballEntity, int screenHeight);
        void Update(VECTOR::Registry& registry, float deltaTime) override;
    private:
        VECTOR::Entity m_BallEntity;
        int m_ScreenHeight;
    };

    class BallMechanicsSystem : public VECTOR::System {
    public:
        BallMechanicsSystem(int screenWidth, int screenHeight, VECTOR::ParticleEmitter* explosionEmitter);
        void Update(VECTOR::Registry& registry, float deltaTime) override;
        
        bool WasLeftScored() const { return m_LeftScored; }
        bool WasRightScored() const { return m_RightScored; }
        void ResetScoreFlags() { m_LeftScored = false; m_RightScored = false; }

        void SetLeftScored() { m_LeftScored = true; }
        void SetRightScored() { m_RightScored = true; }

    private:
        int m_ScreenWidth;
        int m_ScreenHeight;
        bool m_LeftScored = false;
        bool m_RightScored = false;
        VECTOR::ParticleEmitter* m_ExplosionEmitter;
    };

}
