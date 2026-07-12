#include "PongGame.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Audio/AudioManager.hpp"

namespace Game {

    PongGame::PongGame(const std::string& title, int width, int height)
        : VECTOR::Application(title, width, height), m_Score1(0), m_Score2(0),
          m_State(GameState::StartMenu), m_WasPausePressed(false), m_WasEnterPressed(false),
          m_WasF3Pressed(false), m_DebugMode(false)
    {
        // Player 1 (Left) - W/S
        m_Player1 = std::make_unique<Paddle>(30.0f, height / 2.0f - 50.0f, SDL_SCANCODE_W, SDL_SCANCODE_S);
        
        // Player 2 (Right) - AI Agent
        m_Player2 = std::make_unique<AIPaddle>(width - 50.0f, height / 2.0f - 50.0f);

        // Ball (Center)
        m_Ball = std::make_unique<Ball>(width / 2.0f, height / 2.0f);
    }

    PongGame::~PongGame() {}

    void PongGame::Update(float deltaTime) {
        bool isEnterPressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_RETURN);
        bool isPausePressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_P);
        bool isEscPressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_ESCAPE);
        bool isF3Pressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_F3);

        if (isF3Pressed && !m_WasF3Pressed) {
            m_DebugMode = !m_DebugMode;
            VECTOR_LOG_INFO(std::string("Debug Mode: ") + (m_DebugMode ? "ON" : "OFF"));
        }

        if (m_State == GameState::StartMenu) {
            if (m_InputManager->IsKeyPressed(SDL_SCANCODE_1)) {
                m_Player2->SetDifficulty(AIDifficulty::Easy);
            } else if (m_InputManager->IsKeyPressed(SDL_SCANCODE_2)) {
                m_Player2->SetDifficulty(AIDifficulty::Medium);
            } else if (m_InputManager->IsKeyPressed(SDL_SCANCODE_3)) {
                m_Player2->SetDifficulty(AIDifficulty::Hard);
            }

            if (isEnterPressed && !m_WasEnterPressed) {
                // Reset scores and entities when starting a new game
                m_Score1 = 0;
                m_Score2 = 0;
                ResetGame();
                m_State = GameState::Playing;
            }
        } 
        else if (m_State == GameState::Playing) {
            if (isEscPressed) {
                m_State = GameState::StartMenu;
            } else if (isPausePressed && !m_WasPausePressed) {
                m_State = GameState::Paused;
            }

            // Update entities
            m_Player1->Update(deltaTime, m_InputManager.get(), m_Height);
            m_Player2->UpdateAI(deltaTime, m_Ball.get(), m_Height);
            m_Ball->Update(deltaTime, m_Width, m_Height);

            CheckCollisions();

            // Scoring
            if (m_Ball->IsOutOfBoundsLeft()) {
                m_Score2++;
                VECTOR_LOG_INFO(std::string("Player 2 Scored! Score: ") + std::to_string(m_Score1) + " - " + std::to_string(m_Score2));
                ResetGame();
            } else if (m_Ball->IsOutOfBoundsRight(m_Width)) {
                m_Score1++;
                VECTOR_LOG_INFO(std::string("Player 1 Scored! Score: ") + std::to_string(m_Score1) + " - " + std::to_string(m_Score2));
                ResetGame();
            }
        } 
        else if (m_State == GameState::Paused) {
            if (isEscPressed) {
                m_State = GameState::StartMenu;
            } else if (isPausePressed && !m_WasPausePressed) {
                m_State = GameState::Playing;
            }
        }

        m_WasEnterPressed = isEnterPressed;
        m_WasPausePressed = isPausePressed;
        m_WasF3Pressed = isF3Pressed;
    }

    void PongGame::Render() {
        // Clear screen to black
        m_Renderer->Clear(0, 0, 0, 255);

        if (m_State == GameState::StartMenu) {
            m_Renderer->DrawText("PONG", m_Width / 2 - 50, m_Height / 2 - 150, 255, 255, 255, 48);

            AIDifficulty diff = m_Player2->GetDifficulty();
            std::string easyPrefix = (diff == AIDifficulty::Easy) ? "> " : "  ";
            std::string medPrefix  = (diff == AIDifficulty::Medium) ? "> " : "  ";
            std::string hardPrefix = (diff == AIDifficulty::Hard) ? "> " : "  ";

            m_Renderer->DrawText(easyPrefix + "Press 1 for EASY AI", m_Width / 2 - 170, m_Height / 2 - 40, 0, 255, 0, 24);
            m_Renderer->DrawText(medPrefix + "Press 2 for MEDIUM AI", m_Width / 2 - 170, m_Height / 2, 255, 255, 0, 24);
            m_Renderer->DrawText(hardPrefix + "Press 3 for HARD AI", m_Width / 2 - 170, m_Height / 2 + 40, 255, 0, 0, 24);
            
            m_Renderer->DrawText("Press ENTER to Start", m_Width / 2 - 150, m_Height / 2 + 120, 255, 255, 255, 24);
        } else {
            // Draw entities (used in both Playing and Paused)
            m_Player1->Render(m_Renderer.get());
            m_Player2->Render(m_Renderer.get());
            m_Ball->Render(m_Renderer.get());

            // Draw center dashed line
            for (int y = 0; y < m_Height; y += 30) {
                m_Renderer->DrawRect(m_Width / 2 - 2, y, 4, 15, 255, 255, 255, 100);
            }

            // Draw score
            std::string scoreText = std::to_string(m_Score1) + "   " + std::to_string(m_Score2);
            m_Renderer->DrawText(scoreText, m_Width / 2 - 35, 20, 255, 255, 255, 36);

            if (m_State == GameState::Paused) {
                m_Renderer->DrawText("PAUSED", m_Width / 2 - 60, m_Height / 2 - 20, 255, 255, 0, 36);
                m_Renderer->DrawText("Press P to Resume | ESC for Menu", m_Width / 2 - 200, m_Height / 2 + 30, 255, 255, 255, 24);
            }

            if (m_DebugMode) {
                // Draw AABB for all entities
                auto drawAABB = [this](VECTOR::AABB aabb) {
                    m_Renderer->DrawRect(aabb.x, aabb.y, aabb.w, 1, 0, 255, 0); // Top
                    m_Renderer->DrawRect(aabb.x, aabb.y + aabb.h, aabb.w, 1, 0, 255, 0); // Bottom
                    m_Renderer->DrawRect(aabb.x, aabb.y, 1, aabb.h, 0, 255, 0); // Left
                    m_Renderer->DrawRect(aabb.x + aabb.w, aabb.y, 1, aabb.h, 0, 255, 0); // Right
                };

                drawAABB(m_Player1->GetAABB());
                drawAABB(m_Player2->GetAABB());
                drawAABB(m_Ball->GetAABB());

                // Debug Text
                std::string p1Text = m_Player1->GetName() + " Y: " + std::to_string((int)m_Player1->GetPosition().y);
                std::string p2Text = m_Player2->GetName() + " Y: " + std::to_string((int)m_Player2->GetPosition().y);
                std::string ballText = m_Ball->GetName() + " X: " + std::to_string((int)m_Ball->GetPosition().x) + " Y: " + std::to_string((int)m_Ball->GetPosition().y);
                std::string fpsText = "FPS: " + std::to_string((int)GetFPS());
                
                m_Renderer->DrawText(p1Text, 10, 10, 0, 255, 0, 16);
                m_Renderer->DrawText(p2Text, m_Width - 200, 10, 0, 255, 0, 16);
                m_Renderer->DrawText(ballText, m_Width / 2 - 100, m_Height - 30, 0, 255, 255, 16);
                m_Renderer->DrawText(fpsText, 10, m_Height - 30, 255, 255, 0, 16);
            }
        }

        // Present to screen
        m_Renderer->Present();
    }

    void PongGame::CheckCollisions() {
        VECTOR::AABB ballAABB = m_Ball->GetAABB();
        VECTOR::AABB p1AABB = m_Player1->GetAABB();
        VECTOR::AABB p2AABB = m_Player2->GetAABB();

        auto resolveCollision = [this, &ballAABB](VECTOR::AABB& paddleAABB, bool isLeftPaddle) {
            // Calculate relative intersection Y
            float paddleCenterY = paddleAABB.y + (paddleAABB.h / 2.0f);
            float ballCenterY = ballAABB.y + (ballAABB.h / 2.0f);
            float relativeIntersectY = (paddleCenterY - ballCenterY);
            
            // Normalize it (-1.0 to 1.0)
            float normalizedIntersectY = (relativeIntersectY / (paddleAABB.h / 2.0f));
            
            // Max bounce angle (e.g., 60 degrees in radians = 1.047)
            float maxBounceAngle = 1.047f;
            float bounceAngle = normalizedIntersectY * maxBounceAngle;

            // Increase ball speed by 5%, capped at 1000
            float speed = m_Ball->GetSpeed();
            speed = std::min(speed * 1.05f, 1000.0f);
            m_Ball->SetSpeed(speed);

            // Calculate new velocity
            float dirX = isLeftPaddle ? std::cos(bounceAngle) : -std::cos(bounceAngle);
            float dirY = -std::sin(bounceAngle); // Negative because Y goes down in SDL
            
            m_Ball->SetVelocity(dirX * speed, dirY * speed);

            // Prevent getting stuck by pushing the ball out completely
            if (isLeftPaddle) {
                m_Ball->SetPosition(paddleAABB.x + paddleAABB.w + 1.0f, ballAABB.y);
            } else {
                m_Ball->SetPosition(paddleAABB.x - ballAABB.w - 1.0f, ballAABB.y);
            }

            // Play hit sound
            VECTOR::AudioManager::Get().PlaySound("assets/hit.wav");
        };

        if (ballAABB.Intersects(p1AABB)) {
            resolveCollision(p1AABB, true);
        } else if (ballAABB.Intersects(p2AABB)) {
            resolveCollision(p2AABB, false);
        }
    }

    void PongGame::ResetGame() {
        m_Player1->Reset(30.0f, m_Height / 2.0f - 50.0f);
        m_Player2->Reset(m_Width - 50.0f, m_Height / 2.0f - 50.0f);
        m_Ball->Reset(m_Width / 2.0f, m_Height / 2.0f);
    }

} // namespace Game
