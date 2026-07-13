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
        m_Registry.AddComponent(m_Ball, VECTOR::VelocityComponent{{400.0f * 0.707106f, 400.0f * 0.707106f}});
        m_Registry.AddComponent(m_Ball, VECTOR::RenderComponent{15.0f, 15.0f, 255, 255, 255, 255});
        m_Registry.AddComponent(m_Ball, VECTOR::ColliderComponent{15.0f, 15.0f});
        m_Registry.AddComponent(m_Ball, BallComponent{true});

        // Initialize Systems
        m_Systems.push_back(std::make_unique<PlayerInputSystem>(m_InputManager, m_Height));
        m_Systems.push_back(std::make_unique<AISystem>(m_Ball, m_Height));
        m_Systems.push_back(std::make_unique<PhysicsSystem>());
        
        auto ballSys = std::make_unique<BallMechanicsSystem>(m_Width, m_Height, &m_ExplosionEmitter);
        m_BallSystem = ballSys.get();
        m_Systems.push_back(std::move(ballSys));
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
            // Run all ECS Systems
            for (auto& system : m_Systems) {
                system->Update(m_Registry, deltaTime);
            }

            // Emit trail particles
            auto& ballTrans = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Ball);
            auto& ballCol = m_Registry.GetComponent<VECTOR::ColliderComponent>(m_Ball);
            m_TrailEmitter.Emit(ballTrans.position.x + ballCol.width/2.0f, ballTrans.position.y + ballCol.height/2.0f, 1, 255, 200, 50, 20.0f, 0.3f);
            
            m_TrailEmitter.Update(deltaTime);
            m_ExplosionEmitter.Update(deltaTime);

            // Score logic from BallMechanicsSystem
            if (m_BallSystem->WasLeftScored()) {
                m_Score1++;
                VECTOR::EventBus::Get().Publish<ScoreEvent>(1);
                VECTOR_LOG_INFO(std::string("Player 1 Scored! Score: ") + std::to_string(m_Score1) + " - " + std::to_string(m_Score2));
                
                if (m_Score1 >= WINNING_SCORE) {
                    m_State = GameState::GameOver;
                    m_Winner = 1;
                } else {
                    ResetGame();
                }
                m_BallSystem->ResetScoreFlags();
            } else if (m_BallSystem->WasRightScored()) {
                m_Score2++;
                VECTOR::EventBus::Get().Publish<ScoreEvent>(2);
                VECTOR_LOG_INFO(std::string("Player 2 Scored! Score: ") + std::to_string(m_Score1) + " - " + std::to_string(m_Score2));
                
                if (m_Score2 >= WINNING_SCORE) {
                    m_State = GameState::GameOver;
                    m_Winner = 2;
                } else {
                    ResetGame();
                }
                m_BallSystem->ResetScoreFlags();
            }
        }
    }

    void GameplayScene::Render(VECTOR::Renderer* renderer) {
        m_TrailEmitter.Render(renderer);
        m_ExplosionEmitter.Render(renderer);
        
        m_Registry.View<VECTOR::TransformComponent, VECTOR::RenderComponent>([&](VECTOR::Entity entity) {
            auto& transform = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& r = m_Registry.GetComponent<VECTOR::RenderComponent>(entity);
            renderer->DrawRect((int)transform.position.x, (int)transform.position.y, (int)r.width, (int)r.height, r.r, r.g, r.b, r.a);
        });

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
            m_Registry.View<VECTOR::TransformComponent, VECTOR::ColliderComponent>([&](VECTOR::Entity entity) {
                auto& t = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
                auto& c = m_Registry.GetComponent<VECTOR::ColliderComponent>(entity);
                renderer->DrawRect((int)t.position.x, (int)t.position.y, (int)c.width, 1, 0, 255, 0); 
                renderer->DrawRect((int)t.position.x, (int)t.position.y + (int)c.height, (int)c.width, 1, 0, 255, 0); 
                renderer->DrawRect((int)t.position.x, (int)t.position.y, 1, (int)c.height, 0, 255, 0); 
                renderer->DrawRect((int)t.position.x + (int)c.width, (int)t.position.y, 1, (int)c.height, 0, 255, 0); 
            });

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
