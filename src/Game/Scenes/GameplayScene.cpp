#include "GameplayScene.hpp"
#include "MainMenuScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Events/EventBus.hpp"
#include "Game/Events/GameEvents.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/ECS/Components.hpp"
#include <cmath>
#include <algorithm>

namespace Game {

    GameplayScene::GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty)
        : m_Width(width), m_Height(height), m_InputManager(inputManager),
          m_Score1(0), m_Score2(0), m_IsPaused(false), m_WasPausePressed(false), m_DebugMode(false), m_WasF3Pressed(false),
          m_TrailEmitter(200), m_ExplosionEmitter(300), m_State(GameState::Playing), m_Winner(0)
    {
        m_Registry.RegisterComponent<VECTOR::TransformComponent>();
        m_Registry.RegisterComponent<VECTOR::VelocityComponent>();
        m_Registry.RegisterComponent<VECTOR::RenderComponent>();
        m_Registry.RegisterComponent<VECTOR::ColliderComponent>();
        m_Registry.RegisterComponent<PlayerInputComponent>();
        m_Registry.RegisterComponent<AIComponent>();
        m_Registry.RegisterComponent<BallComponent>();

        m_Player1 = m_Registry.CreateEntity();
        m_Registry.AddComponent(m_Player1, VECTOR::TransformComponent{{30.0f, height / 2.0f - 50.0f}});
        m_Registry.AddComponent(m_Player1, VECTOR::VelocityComponent{{0.0f, 0.0f}});
        m_Registry.AddComponent(m_Player1, VECTOR::RenderComponent{20.0f, 100.0f, 255, 255, 255, 255});
        m_Registry.AddComponent(m_Player1, VECTOR::ColliderComponent{20.0f, 100.0f});
        m_Registry.AddComponent(m_Player1, PlayerInputComponent{SDL_SCANCODE_W, SDL_SCANCODE_S});

        m_Player2 = m_Registry.CreateEntity();
        m_Registry.AddComponent(m_Player2, VECTOR::TransformComponent{{width - 50.0f, height / 2.0f - 50.0f}});
        m_Registry.AddComponent(m_Player2, VECTOR::VelocityComponent{{0.0f, 0.0f}});
        m_Registry.AddComponent(m_Player2, VECTOR::RenderComponent{20.0f, 100.0f, 255, 255, 255, 255});
        m_Registry.AddComponent(m_Player2, VECTOR::ColliderComponent{20.0f, 100.0f});
        m_Registry.AddComponent(m_Player2, AIComponent{aiDifficulty, AIState::Idle, 0.0f, 0.2f, height / 2.0f});

        m_Ball = m_Registry.CreateEntity();
        m_Registry.AddComponent(m_Ball, VECTOR::TransformComponent{{width / 2.0f, height / 2.0f}});
        m_Registry.AddComponent(m_Ball, VECTOR::VelocityComponent{{400.0f * 0.707106f, 400.0f * 0.707106f}}); // Init speed
        m_Registry.AddComponent(m_Ball, VECTOR::RenderComponent{15.0f, 15.0f, 255, 255, 255, 255});
        m_Registry.AddComponent(m_Ball, VECTOR::ColliderComponent{15.0f, 15.0f});
        m_Registry.AddComponent(m_Ball, BallComponent{true});
    }

    GameplayScene::~GameplayScene() {}

    void GameplayScene::OnEnter() {
        ResetGame();
        m_WasPausePressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_P);
    }

    void GameplayScene::Update(float deltaTime) {
        bool isPausePressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_P);
        bool isEscPressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_ESCAPE);
        bool isF3Pressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_F3);

        if (isF3Pressed && !m_WasF3Pressed) {
            m_DebugMode = !m_DebugMode;
            VECTOR_LOG_INFO(std::string("Debug Mode: ") + (m_DebugMode ? "ON" : "OFF"));
        }
        m_WasF3Pressed = isF3Pressed;

        if (isEscPressed) {
            auto menuScene = std::make_unique<MainMenuScene>(m_Width, m_Height, m_InputManager);
            VECTOR::SceneManager::Get().ChangeScene(std::move(menuScene));
            return;
        }

        if (m_State == GameState::GameOver) {
            return; // Don't update game logic if over
        }

        if (isPausePressed && !m_WasPausePressed) {
            m_IsPaused = !m_IsPaused;
        }
        m_WasPausePressed = isPausePressed;

        if (!m_IsPaused) {
            // === SYSTEM: Player Input ===
            auto players = m_Registry.View<VECTOR::TransformComponent, PlayerInputComponent>();
            for (auto entity : players) {
                auto& transform = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
                auto& input = m_Registry.GetComponent<PlayerInputComponent>(entity);
                float speed = 500.0f;
                if (m_InputManager->IsKeyPressed(input.upKey)) {
                    transform.position.y -= speed * deltaTime;
                }
                if (m_InputManager->IsKeyPressed(input.downKey)) {
                    transform.position.y += speed * deltaTime;
                }
                // Clamp
                if (transform.position.y < 0.0f) transform.position.y = 0.0f;
                if (transform.position.y + 100.0f > m_Height) transform.position.y = m_Height - 100.0f;
            }

            // === SYSTEM: AI ===
            auto ais = m_Registry.View<VECTOR::TransformComponent, AIComponent>();
            for (auto entity : ais) {
                auto& transform = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
                auto& ai = m_Registry.GetComponent<AIComponent>(entity);
                
                auto& ballTrans = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Ball);
                auto& ballVel = m_Registry.GetComponent<VECTOR::VelocityComponent>(m_Ball);

                if (ballVel.velocity.x < 0.0f) {
                    ai.state = AIState::Idle;
                } else {
                    ai.state = (ai.difficulty == AIDifficulty::Hard) ? AIState::Predicting : AIState::Tracking;
                }

                ai.reactionDelayTimer += deltaTime;
                if (ai.reactionDelayTimer >= ai.reactionTime) {
                    switch (ai.state) {
                        case AIState::Idle:
                            ai.targetY = (m_Height / 2.0f);
                            break;
                        case AIState::Tracking:
                            ai.targetY = ballTrans.position.y + 7.5f; // center of ball
                            if (ai.difficulty == AIDifficulty::Easy) ai.targetY += (rand() % 60) - 30;
                            else if (ai.difficulty == AIDifficulty::Medium) ai.targetY += (rand() % 30) - 15;
                            break;
                        case AIState::Predicting:
                            {
                                float timeToReach = (transform.position.x - (ballTrans.position.x + 15.0f)) / ballVel.velocity.x;
                                if (timeToReach < 0.0f) timeToReach = 0.0f;
                                float expectedY = ballTrans.position.y + (ballVel.velocity.y * timeToReach);
                                int maxBounces = 10;
                                while (maxBounces > 0 && (expectedY < 0.0f || expectedY + 15.0f > m_Height)) {
                                    if (expectedY < 0.0f) expectedY = -expectedY;
                                    else if (expectedY + 15.0f > m_Height) expectedY = 2.0f * (m_Height - 15.0f) - expectedY;
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
                if (transform.position.y + 100.0f > m_Height) transform.position.y = m_Height - 100.0f;
            }

            // === SYSTEM: Physics Movement ===
            auto movables = m_Registry.View<VECTOR::TransformComponent, VECTOR::VelocityComponent>();
            for (auto entity : movables) {
                auto& transform = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
                auto& vel = m_Registry.GetComponent<VECTOR::VelocityComponent>(entity);
                transform.position.x += vel.velocity.x * deltaTime;
                transform.position.y += vel.velocity.y * deltaTime;
            }

            // === SYSTEM: Ball Mechanics ===
            auto& ballTrans = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Ball);
            auto& ballVel = m_Registry.GetComponent<VECTOR::VelocityComponent>(m_Ball);
            auto& ballCol = m_Registry.GetComponent<VECTOR::ColliderComponent>(m_Ball);

            // Bounce Y
            if (ballTrans.position.y < 0.0f) {
                ballTrans.position.y = 0.0f;
                ballVel.velocity.y = -ballVel.velocity.y;
            } else if (ballTrans.position.y + ballCol.height > m_Height) {
                ballTrans.position.y = m_Height - ballCol.height;
                ballVel.velocity.y = -ballVel.velocity.y;
            }

            // Trail particles
            m_TrailEmitter.Emit(ballTrans.position.x + ballCol.width/2.0f, ballTrans.position.y + ballCol.height/2.0f, 1, 255, 200, 50, 20.0f, 0.3f);
            m_TrailEmitter.Update(deltaTime);
            m_ExplosionEmitter.Update(deltaTime);

            CheckCollisions();

            // Score logic
            if (ballTrans.position.x < 0.0f) {
                m_Score2++;
                VECTOR::EventBus::Get().Publish<ScoreEvent>(2);
                VECTOR_LOG_INFO(std::string("Player 2 Scored! Score: ") + std::to_string(m_Score1) + " - " + std::to_string(m_Score2));
                
                if (m_Score2 >= WINNING_SCORE) {
                    m_State = GameState::GameOver;
                    m_Winner = 2;
                } else {
                    ResetGame();
                }
            } else if (ballTrans.position.x + ballCol.width > m_Width) {
                m_Score1++;
                VECTOR::EventBus::Get().Publish<ScoreEvent>(1);
                VECTOR_LOG_INFO(std::string("Player 1 Scored! Score: ") + std::to_string(m_Score1) + " - " + std::to_string(m_Score2));
                
                if (m_Score1 >= WINNING_SCORE) {
                    m_State = GameState::GameOver;
                    m_Winner = 1;
                } else {
                    ResetGame();
                }
            }
        }
    }

    void GameplayScene::Render(VECTOR::Renderer* renderer) {
        m_TrailEmitter.Render(renderer);
        m_ExplosionEmitter.Render(renderer);
        
        // === SYSTEM: Render ===
        auto renderables = m_Registry.View<VECTOR::TransformComponent, VECTOR::RenderComponent>();
        for (auto entity : renderables) {
            auto& transform = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& r = m_Registry.GetComponent<VECTOR::RenderComponent>(entity);
            renderer->DrawRect((int)transform.position.x, (int)transform.position.y, (int)r.width, (int)r.height, r.r, r.g, r.b, r.a);
        }

        for (int y = 0; y < m_Height; y += 30) {
            renderer->DrawRect(m_Width / 2 - 2, y, 4, 15, 255, 255, 255, 100);
        }

        renderer->DrawText(std::to_string(m_Score1), m_Width / 2 - 50, 20, 255, 255, 255, 48);
        renderer->DrawText(std::to_string(m_Score2), m_Width / 2 + 20, 20, 255, 255, 255, 48);

        if (m_State == GameState::GameOver) {
            std::string winText = (m_Winner == 1) ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
            renderer->DrawText(winText, m_Width / 2 - 140, m_Height / 2 - 50, 255, 215, 0, 48);
            renderer->DrawText("Press ESC to return to Menu", m_Width / 2 - 160, m_Height / 2 + 20, 255, 255, 255, 24);
        } else if (m_IsPaused) {
            renderer->DrawText("PAUSED", m_Width / 2 - 60, m_Height / 2 - 24, 255, 255, 255, 48);
            renderer->DrawText("Press P to Resume | ESC for Menu", m_Width / 2 - 200, m_Height / 2 + 30, 255, 255, 255, 24);
        }

        if (m_DebugMode) {
            auto colliders = m_Registry.View<VECTOR::TransformComponent, VECTOR::ColliderComponent>();
            for (auto entity : colliders) {
                auto& t = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
                auto& c = m_Registry.GetComponent<VECTOR::ColliderComponent>(entity);
                renderer->DrawRect((int)t.position.x, (int)t.position.y, (int)c.width, 1, 0, 255, 0); 
                renderer->DrawRect((int)t.position.x, (int)t.position.y + (int)c.height, (int)c.width, 1, 0, 255, 0); 
                renderer->DrawRect((int)t.position.x, (int)t.position.y, 1, (int)c.height, 0, 255, 0); 
                renderer->DrawRect((int)t.position.x + (int)c.width, (int)t.position.y, 1, (int)c.height, 0, 255, 0); 
            }

            auto& p1T = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Player1);
            auto& p2T = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Player2);
            auto& bT = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Ball);

            std::string p1Text = "P1 Y: " + std::to_string((int)p1T.position.y);
            std::string p2Text = "P2 Y: " + std::to_string((int)p2T.position.y);
            std::string ballText = "Ball X: " + std::to_string((int)bT.position.x) + " Y: " + std::to_string((int)bT.position.y);
            std::string fpsText = "FPS: " + std::to_string((int)VECTOR::Application::Get().GetFPS());
            
            renderer->DrawText(p1Text, 10, 10, 0, 255, 0, 16);
            renderer->DrawText(p2Text, m_Width - 200, 10, 0, 255, 0, 16);
            renderer->DrawText(fpsText, m_Width - 100, 30, 255, 255, 0, 16);
            renderer->DrawText(ballText, m_Width / 2 - 100, m_Height - 30, 0, 255, 255, 16);
        }
    }

    void GameplayScene::CheckCollisions() {
        auto& ballTrans = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Ball);
        auto& ballCol = m_Registry.GetComponent<VECTOR::ColliderComponent>(m_Ball);
        auto& ballVel = m_Registry.GetComponent<VECTOR::VelocityComponent>(m_Ball);

        auto checkIntersect = [&](VECTOR::TransformComponent& t1, VECTOR::ColliderComponent& c1, VECTOR::TransformComponent& t2, VECTOR::ColliderComponent& c2) {
            return t1.position.x < t2.position.x + c2.width &&
                   t1.position.x + c1.width > t2.position.x &&
                   t1.position.y < t2.position.y + c2.height &&
                   t1.position.y + c1.height > t2.position.y;
        };

        auto resolveCollision = [&](VECTOR::Entity paddleEntity, bool isLeftPaddle) {
            auto& paddleTrans = m_Registry.GetComponent<VECTOR::TransformComponent>(paddleEntity);
            auto& paddleCol = m_Registry.GetComponent<VECTOR::ColliderComponent>(paddleEntity);

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
            m_ExplosionEmitter.Emit(explosionX, ballCenterY, 30, 255, 100, 50, 300.0f, 0.5f);

            VECTOR::EventBus::Get().Publish<CollisionEvent>();
        };

        auto& p1Trans = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Player1);
        auto& p1Col = m_Registry.GetComponent<VECTOR::ColliderComponent>(m_Player1);
        if (checkIntersect(ballTrans, ballCol, p1Trans, p1Col)) {
            resolveCollision(m_Player1, true);
        } else {
            auto& p2Trans = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Player2);
            auto& p2Col = m_Registry.GetComponent<VECTOR::ColliderComponent>(m_Player2);
            if (checkIntersect(ballTrans, ballCol, p2Trans, p2Col)) {
                resolveCollision(m_Player2, false);
            }
        }
    }

    void GameplayScene::ResetGame() {
        m_Registry.GetComponent<VECTOR::TransformComponent>(m_Player1).position = VECTOR::Vector2D(30.0f, m_Height / 2.0f - 50.0f);
        m_Registry.GetComponent<VECTOR::TransformComponent>(m_Player2).position = VECTOR::Vector2D(m_Width - 50.0f, m_Height / 2.0f - 50.0f);
        
        auto& ballTrans = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Ball);
        auto& ballVel = m_Registry.GetComponent<VECTOR::VelocityComponent>(m_Ball);
        ballTrans.position = VECTOR::Vector2D(m_Width / 2.0f, m_Height / 2.0f);
        
        float dirX = ballVel.velocity.x > 0 ? -0.707106f : 0.707106f;
        float dirY = ballVel.velocity.y > 0 ? -0.707106f : 0.707106f;
        ballVel.velocity = VECTOR::Vector2D(dirX * 400.0f, dirY * 400.0f);
    }

} // namespace Game
