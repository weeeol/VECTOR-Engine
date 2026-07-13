#include "GameSystems.hpp"
#include "Engine/ECS/Components.hpp"
#include "Game/Core/GameComponents.hpp"
#include "Engine/Events/EventBus.hpp"
#include "Game/Events/GameEvents.hpp"
#include <cmath>
#include <algorithm>

namespace Game {

    // --- Player Input System ---
    PlayerInputSystem::PlayerInputSystem(VECTOR::InputManager* inputManager, int screenHeight)
        : m_InputManager(inputManager), m_ScreenHeight(screenHeight) {}

    void PlayerInputSystem::Update(VECTOR::Registry& registry, float deltaTime) {
        registry.View<VECTOR::TransformComponent, PlayerInputComponent>([&](VECTOR::Entity entity) {
            auto& transform = registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& input = registry.GetComponent<PlayerInputComponent>(entity);
            float speed = 500.0f;
            
            if (m_InputManager->IsKeyPressed(input.upKey)) {
                transform.position.y -= speed * deltaTime;
            }
            if (m_InputManager->IsKeyPressed(input.downKey)) {
                transform.position.y += speed * deltaTime;
            }
            
            // Clamp
            if (transform.position.y < 0.0f) transform.position.y = 0.0f;
            if (transform.position.y + 100.0f > m_ScreenHeight) transform.position.y = m_ScreenHeight - 100.0f;
        });
    }

    // --- AI System ---
    AISystem::AISystem(VECTOR::Entity ballEntity, int screenHeight)
        : m_BallEntity(ballEntity), m_ScreenHeight(screenHeight) {}

    void AISystem::Update(VECTOR::Registry& registry, float deltaTime) {
        if (!registry.HasComponent<VECTOR::TransformComponent>(m_BallEntity) || !registry.HasComponent<VECTOR::VelocityComponent>(m_BallEntity)) return;
        
        auto& ballTrans = registry.GetComponent<VECTOR::TransformComponent>(m_BallEntity);
        auto& ballVel = registry.GetComponent<VECTOR::VelocityComponent>(m_BallEntity);

        registry.View<VECTOR::TransformComponent, AIComponent>([&](VECTOR::Entity entity) {
            auto& transform = registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& ai = registry.GetComponent<AIComponent>(entity);
            
            if (ballVel.velocity.x < 0.0f) {
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
                        ai.targetY = ballTrans.position.y + 7.5f;
                        if (ai.difficulty == AIDifficulty::Easy) ai.targetY += (rand() % 60) - 30;
                        else if (ai.difficulty == AIDifficulty::Medium) ai.targetY += (rand() % 30) - 15;
                        break;
                    case AIState::Predicting:
                        {
                            float timeToReach = (transform.position.x - (ballTrans.position.x + 15.0f)) / ballVel.velocity.x;
                            if (timeToReach < 0.0f) timeToReach = 0.0f;
                            float expectedY = ballTrans.position.y + (ballVel.velocity.y * timeToReach);
                            int maxBounces = 10;
                            while (maxBounces > 0 && (expectedY < 0.0f || expectedY + 15.0f > m_ScreenHeight)) {
                                if (expectedY < 0.0f) expectedY = -expectedY;
                                else if (expectedY + 15.0f > m_ScreenHeight) expectedY = 2.0f * (m_ScreenHeight - 15.0f) - expectedY;
                                maxBounces--;
                            }
                            ai.targetY = expectedY + 7.5f;
                        }
                        break;
                }
                ai.reactionDelayTimer = 0.0f;
            }

            float aiSpeed = 400.0f;
            float paddleCenterY = transform.position.y + 50.0f;
            if (paddleCenterY < ai.targetY - 5.0f) transform.position.y += aiSpeed * deltaTime;
            else if (paddleCenterY > ai.targetY + 5.0f) transform.position.y -= aiSpeed * deltaTime;

            if (transform.position.y < 0.0f) transform.position.y = 0.0f;
            if (transform.position.y + 100.0f > m_ScreenHeight) transform.position.y = m_ScreenHeight - 100.0f;
        });
    }

    // --- Physics System ---
    void PhysicsSystem::Update(VECTOR::Registry& registry, float deltaTime) {
        registry.View<VECTOR::TransformComponent, VECTOR::VelocityComponent>([&](VECTOR::Entity entity) {
            auto& transform = registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& vel = registry.GetComponent<VECTOR::VelocityComponent>(entity);
            transform.position.x += vel.velocity.x * deltaTime;
            transform.position.y += vel.velocity.y * deltaTime;
        });
    }

    // --- Ball Mechanics System ---
    BallMechanicsSystem::BallMechanicsSystem(int screenWidth, int screenHeight, VECTOR::ParticleEmitter* explosionEmitter)
        : m_ScreenWidth(screenWidth), m_ScreenHeight(screenHeight), m_ExplosionEmitter(explosionEmitter) {}

    void BallMechanicsSystem::Update(VECTOR::Registry& registry, float deltaTime) {
        registry.View<VECTOR::TransformComponent, VECTOR::VelocityComponent, VECTOR::ColliderComponent, BallComponent>([&](VECTOR::Entity ballEntity) {
            auto& ballTrans = registry.GetComponent<VECTOR::TransformComponent>(ballEntity);
            auto& ballVel = registry.GetComponent<VECTOR::VelocityComponent>(ballEntity);
            auto& ballCol = registry.GetComponent<VECTOR::ColliderComponent>(ballEntity);

            // 1. Boundary Y
            if (ballTrans.position.y < 0.0f) {
                ballTrans.position.y = 0.0f;
                ballVel.velocity.y = -ballVel.velocity.y;
            } else if (ballTrans.position.y + ballCol.height > m_ScreenHeight) {
                ballTrans.position.y = m_ScreenHeight - ballCol.height;
                ballVel.velocity.y = -ballVel.velocity.y;
            }

            // 2. Score check
            if (ballTrans.position.x < 0.0f) {
                m_RightScored = true;
            } else if (ballTrans.position.x + ballCol.width > m_ScreenWidth) {
                m_LeftScored = true;
            }

            // 3. Paddle Collisions
            auto checkIntersect = [&](VECTOR::TransformComponent& t1, VECTOR::ColliderComponent& c1, VECTOR::TransformComponent& t2, VECTOR::ColliderComponent& c2) {
                return t1.position.x < t2.position.x + c2.width &&
                       t1.position.x + c1.width > t2.position.x &&
                       t1.position.y < t2.position.y + c2.height &&
                       t1.position.y + c1.height > t2.position.y;
            };

            auto resolveCollision = [&](VECTOR::TransformComponent& paddleTrans, VECTOR::ColliderComponent& paddleCol, bool isLeftPaddle) {
                float paddleCenterY = paddleTrans.position.y + (paddleCol.height / 2.0f);
                float ballCenterY = ballTrans.position.y + (ballCol.height / 2.0f);
                float relativeIntersectY = (paddleCenterY - ballCenterY);
                float normalizedIntersectY = (relativeIntersectY / (paddleCol.height / 2.0f));
                float maxBounceAngle = 1.047f;
                float bounceAngle = normalizedIntersectY * maxBounceAngle;

                float currentSpeed = std::sqrt(ballVel.velocity.x * ballVel.velocity.x + ballVel.velocity.y * ballVel.velocity.y);
                currentSpeed = std::min(currentSpeed * 1.05f, 1000.0f);

                float dirX = isLeftPaddle ? std::cos(bounceAngle) : -std::cos(bounceAngle);
                float dirY = -std::sin(bounceAngle);
                
                ballVel.velocity.x = dirX * currentSpeed;
                ballVel.velocity.y = dirY * currentSpeed;

                if (isLeftPaddle) {
                    ballTrans.position.x = paddleTrans.position.x + paddleCol.width + 1.0f;
                } else {
                    ballTrans.position.x = paddleTrans.position.x - ballCol.width - 1.0f;
                }

                float explosionX = isLeftPaddle ? (paddleTrans.position.x + paddleCol.width) : paddleTrans.position.x;
                if (m_ExplosionEmitter) m_ExplosionEmitter->Emit(explosionX, ballCenterY, 30, 255, 100, 50, 300.0f, 0.5f);

                VECTOR::EventBus::Get().Publish<CollisionEvent>();
            };

            // Loop over all non-ball colliders (paddles)
            registry.View<VECTOR::TransformComponent, VECTOR::ColliderComponent>([&](VECTOR::Entity otherEntity) {
                if (otherEntity == ballEntity) return;
                auto& otherTrans = registry.GetComponent<VECTOR::TransformComponent>(otherEntity);
                auto& otherCol = registry.GetComponent<VECTOR::ColliderComponent>(otherEntity);
                
                if (checkIntersect(ballTrans, ballCol, otherTrans, otherCol)) {
                    bool isLeft = (otherTrans.position.x < m_ScreenWidth / 2.0f);
                    resolveCollision(otherTrans, otherCol, isLeft);
                }
            });
        });
    }

} // namespace Game
