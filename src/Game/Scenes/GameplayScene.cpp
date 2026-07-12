#include "GameplayScene.hpp"
#include "MainMenuScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Events/EventBus.hpp"
#include "Game/Events/GameEvents.hpp"
#include "Engine/Core/Logger.hpp"

namespace Game {

    GameplayScene::GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty)
        : m_Width(width), m_Height(height), m_InputManager(inputManager),
          m_Score1(0), m_Score2(0), m_IsPaused(false), m_WasPausePressed(false), m_DebugMode(false), m_WasF3Pressed(false),
          m_TrailEmitter(200), m_ExplosionEmitter(300)
    {
        m_Player1 = std::make_unique<Paddle>(30.0f, height / 2.0f - 50.0f, SDL_SCANCODE_W, SDL_SCANCODE_S);
        m_Player2 = std::make_unique<AIPaddle>(width - 50.0f, height / 2.0f - 50.0f);
        m_Player2->SetDifficulty(aiDifficulty);
        m_Ball = std::make_unique<Ball>(width / 2.0f, height / 2.0f);
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

        if (isPausePressed && !m_WasPausePressed) {
            m_IsPaused = !m_IsPaused;
        }
        m_WasPausePressed = isPausePressed;

        if (!m_IsPaused) {
            m_Player1->Update(deltaTime, m_InputManager, m_Height);
            m_Player2->UpdateAI(deltaTime, m_Ball.get(), m_Height);
            m_Ball->Update(deltaTime, m_Width, m_Height);

            // Emit trail particles from ball center
            VECTOR::AABB ballBox = m_Ball->GetAABB();
            m_TrailEmitter.Emit(ballBox.x + ballBox.w/2.0f, ballBox.y + ballBox.h/2.0f, 1, 255, 200, 50, 20.0f, 0.3f);

            m_TrailEmitter.Update(deltaTime);
            m_ExplosionEmitter.Update(deltaTime);

            CheckCollisions();

            if (m_Ball->IsOutOfBoundsLeft()) {
                m_Score2++;
                VECTOR::EventBus::Get().Publish<ScoreEvent>(2);
                VECTOR_LOG_INFO(std::string("Player 2 Scored! Score: ") + std::to_string(m_Score1) + " - " + std::to_string(m_Score2));
                ResetGame();
            } else if (m_Ball->IsOutOfBoundsRight(m_Width)) {
                m_Score1++;
                VECTOR::EventBus::Get().Publish<ScoreEvent>(1);
                VECTOR_LOG_INFO(std::string("Player 1 Scored! Score: ") + std::to_string(m_Score1) + " - " + std::to_string(m_Score2));
                ResetGame();
            }
        }
    }

    void GameplayScene::Render(VECTOR::Renderer* renderer) {
        m_TrailEmitter.Render(renderer);
        m_ExplosionEmitter.Render(renderer);
        
        m_Player1->Render(renderer);
        m_Player2->Render(renderer);
        m_Ball->Render(renderer);

        for (int y = 0; y < m_Height; y += 30) {
            renderer->DrawRect(m_Width / 2 - 2, y, 4, 15, 255, 255, 255, 100);
        }

        std::string scoreText = std::to_string(m_Score1) + "   " + std::to_string(m_Score2);
        renderer->DrawText(scoreText, m_Width / 2 - 35, 20, 255, 255, 255, 36);

        if (m_IsPaused) {
            renderer->DrawText("PAUSED", m_Width / 2 - 60, m_Height / 2 - 20, 255, 255, 0, 36);
            renderer->DrawText("Press P to Resume | ESC for Menu", m_Width / 2 - 200, m_Height / 2 + 30, 255, 255, 255, 24);
        }

        if (m_DebugMode) {
            auto drawAABB = [renderer](VECTOR::AABB aabb) {
                renderer->DrawRect(aabb.x, aabb.y, aabb.w, 1, 0, 255, 0); 
                renderer->DrawRect(aabb.x, aabb.y + aabb.h, aabb.w, 1, 0, 255, 0); 
                renderer->DrawRect(aabb.x, aabb.y, 1, aabb.h, 0, 255, 0); 
                renderer->DrawRect(aabb.x + aabb.w, aabb.y, 1, aabb.h, 0, 255, 0); 
            };

            drawAABB(m_Player1->GetAABB());
            drawAABB(m_Player2->GetAABB());
            drawAABB(m_Ball->GetAABB());

            std::string p1Text = m_Player1->GetName() + " Y: " + std::to_string((int)m_Player1->GetPosition().y);
            std::string p2Text = m_Player2->GetName() + " Y: " + std::to_string((int)m_Player2->GetPosition().y);
            std::string ballText = m_Ball->GetName() + " X: " + std::to_string((int)m_Ball->GetPosition().x) + " Y: " + std::to_string((int)m_Ball->GetPosition().y);
            
            renderer->DrawText(p1Text, 10, 10, 0, 255, 0, 16);
            renderer->DrawText(p2Text, m_Width - 200, 10, 0, 255, 0, 16);
            renderer->DrawText(ballText, m_Width / 2 - 100, m_Height - 30, 0, 255, 255, 16);
        }
    }

    void GameplayScene::CheckCollisions() {
        VECTOR::AABB ballAABB = m_Ball->GetAABB();
        VECTOR::AABB p1AABB = m_Player1->GetAABB();
        VECTOR::AABB p2AABB = m_Player2->GetAABB();

        auto resolveCollision = [this, &ballAABB](VECTOR::AABB& paddleAABB, bool isLeftPaddle) {
            float paddleCenterY = paddleAABB.y + (paddleAABB.h / 2.0f);
            float ballCenterY = ballAABB.y + (ballAABB.h / 2.0f);
            float relativeIntersectY = (paddleCenterY - ballCenterY);
            float normalizedIntersectY = (relativeIntersectY / (paddleAABB.h / 2.0f));
            float maxBounceAngle = 1.047f;
            float bounceAngle = normalizedIntersectY * maxBounceAngle;

            float speed = m_Ball->GetSpeed();
            speed = std::min(speed * 1.05f, 1000.0f);
            m_Ball->SetSpeed(speed);

            float dirX = isLeftPaddle ? std::cos(bounceAngle) : -std::cos(bounceAngle);
            float dirY = -std::sin(bounceAngle);
            
            m_Ball->SetVelocity(dirX * speed, dirY * speed);

            if (isLeftPaddle) {
                m_Ball->SetPosition(paddleAABB.x + paddleAABB.w + 1.0f, ballAABB.y);
            } else {
                m_Ball->SetPosition(paddleAABB.x - ballAABB.w - 1.0f, ballAABB.y);
            }

            // Fire explosion particles
            float explosionX = isLeftPaddle ? (paddleAABB.x + paddleAABB.w) : paddleAABB.x;
            m_ExplosionEmitter.Emit(explosionX, ballCenterY, 30, 255, 100, 50, 300.0f, 0.5f);

            VECTOR::EventBus::Get().Publish<CollisionEvent>();
        };

        if (ballAABB.Intersects(p1AABB)) {
            resolveCollision(p1AABB, true);
        } else if (ballAABB.Intersects(p2AABB)) {
            resolveCollision(p2AABB, false);
        }
    }

    void GameplayScene::ResetGame() {
        m_Player1->Reset(30.0f, m_Height / 2.0f - 50.0f);
        m_Player2->Reset(m_Width - 50.0f, m_Height / 2.0f - 50.0f);
        m_Ball->Reset(m_Width / 2.0f, m_Height / 2.0f);
    }

} // namespace Game
