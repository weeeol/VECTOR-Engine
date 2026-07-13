#include "GameSystems.hpp"
#include "Engine/ECS/Components.hpp"
#include "Game/Core/GameComponents.hpp"
#include "Engine/Events/EventBus.hpp"
#include "Game/Events/GameEvents.hpp"
#include "Engine/Physics/Box2DPhysicsSystem.hpp"
#include <cmath>
#include <algorithm>

namespace Game {

    PlayerInputSystem::PlayerInputSystem(VECTOR::InputManager* inputManager, int screenHeight)
        : m_InputManager(inputManager), m_ScreenHeight(screenHeight) {}

    void PlayerInputSystem::Update(VECTOR::Registry& registry, float deltaTime) {
        registry.View<VECTOR::RigidBodyComponent, PlayerInputComponent>([&](VECTOR::Entity entity) {
            auto& rigidBody = registry.GetComponent<VECTOR::RigidBodyComponent>(entity);
            auto& input = registry.GetComponent<PlayerInputComponent>(entity);
            float speed = 10.0f; 
            float paddleHalfHeight = 50.0f;
            
            b2Vec2 velocity = {0.0f, 0.0f};
            
            if (b2Body_IsValid(rigidBody.bodyId)) {
                b2Vec2 pos = b2Body_GetPosition(rigidBody.bodyId);
                float paddleY = pos.y * VECTOR::PIXELS_PER_METER;

                if (m_InputManager->IsKeyPressed(input.upKey) && paddleY - paddleHalfHeight > 0) velocity.y = -speed;
                if (m_InputManager->IsKeyPressed(input.downKey) && paddleY + paddleHalfHeight < m_ScreenHeight) velocity.y = speed;
                
                b2Body_SetLinearVelocity(rigidBody.bodyId, velocity);
            }
        });
    }

    AISystem::AISystem(VECTOR::Entity ballEntity, int screenHeight)
        : m_BallEntity(ballEntity), m_ScreenHeight(screenHeight) {}

    void AISystem::Update(VECTOR::Registry& registry, float deltaTime) {
        if (!registry.HasComponent<VECTOR::RigidBodyComponent>(m_BallEntity)) return;
        
        auto& ballRB = registry.GetComponent<VECTOR::RigidBodyComponent>(m_BallEntity);
        if (!b2Body_IsValid(ballRB.bodyId)) return;
        
        b2Vec2 ballPos = b2Body_GetPosition(ballRB.bodyId);
        b2Vec2 ballVel = b2Body_GetLinearVelocity(ballRB.bodyId);

        registry.View<VECTOR::RigidBodyComponent, AIComponent>([&](VECTOR::Entity entity) {
            auto& rigidBody = registry.GetComponent<VECTOR::RigidBodyComponent>(entity);
            auto& ai = registry.GetComponent<AIComponent>(entity);
            if (!b2Body_IsValid(rigidBody.bodyId)) return;
            
            if (ballVel.x < 0.0f) {
                ai.state = AIState::Idle;
            } else {
                ai.state = (ai.difficulty == AIDifficulty::Hard) ? AIState::Predicting : AIState::Tracking;
            }

            ai.reactionDelayTimer += deltaTime;
            if (ai.reactionDelayTimer >= ai.reactionTime) {
                switch (ai.state) {
                    case AIState::Idle:
                        ai.targetY = (m_ScreenHeight / 2.0f);
                        break;
                    case AIState::Tracking:
                        ai.targetY = ballPos.y * VECTOR::PIXELS_PER_METER;
                        if (ai.difficulty == AIDifficulty::Easy) ai.targetY += (rand() % 60) - 30;
                        else if (ai.difficulty == AIDifficulty::Medium) ai.targetY += (rand() % 30) - 15;
                        break;
                    case AIState::Predicting:
                        ai.targetY = ballPos.y * VECTOR::PIXELS_PER_METER;
                        break;
                }
                ai.reactionDelayTimer = 0.0f;
            }

            float aiSpeed = 8.0f;
            b2Vec2 paddlePos = b2Body_GetPosition(rigidBody.bodyId);
            float paddleCenterY = paddlePos.y * VECTOR::PIXELS_PER_METER;
            float paddleHalfHeight = 50.0f;
            
            b2Vec2 velocity = {0.0f, 0.0f};
            if (paddleCenterY < ai.targetY - 5.0f && paddleCenterY + paddleHalfHeight < m_ScreenHeight) velocity.y = aiSpeed;
            else if (paddleCenterY > ai.targetY + 5.0f && paddleCenterY - paddleHalfHeight > 0) velocity.y = -aiSpeed;
            
            b2Body_SetLinearVelocity(rigidBody.bodyId, velocity);
        });
    }

    BallMechanicsSystem::BallMechanicsSystem(int screenWidth, int screenHeight, VECTOR::ParticleEmitter* explosionEmitter)
        : m_ScreenWidth(screenWidth), m_ScreenHeight(screenHeight), m_ExplosionEmitter(explosionEmitter) {}

    void BallMechanicsSystem::Update(VECTOR::Registry& registry, float deltaTime) {
        registry.View<VECTOR::RigidBodyComponent, BallComponent>([&](VECTOR::Entity ballEntity) {
            auto& rigidBody = registry.GetComponent<VECTOR::RigidBodyComponent>(ballEntity);
            if (b2Body_IsValid(rigidBody.bodyId)) {
                b2Vec2 vel = b2Body_GetLinearVelocity(rigidBody.bodyId);
                float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
                if (speed < 6.0f && speed > 0.1f) {
                    vel.x = (vel.x / speed) * 8.0f;
                    vel.y = (vel.y / speed) * 8.0f;
                    b2Body_SetLinearVelocity(rigidBody.bodyId, vel);
                } else if (speed > 30.0f) {
                    vel.x = (vel.x / speed) * 30.0f;
                    vel.y = (vel.y / speed) * 30.0f;
                    b2Body_SetLinearVelocity(rigidBody.bodyId, vel);
                }
            }
        });
    }

} // namespace Game
