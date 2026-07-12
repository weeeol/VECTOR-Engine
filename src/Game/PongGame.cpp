#include "PongGame.hpp"
#include <iostream>

namespace Game {

    PongGame::PongGame(const std::string& title, int width, int height)
        : VECTOR::Application(title, width, height), m_Score1(0), m_Score2(0),
          m_State(GameState::StartMenu), m_WasPausePressed(false), m_WasEnterPressed(false)
    {
        // Player 1 (Left) - W/S
        m_Player1 = std::make_unique<Paddle>(30.0f, height / 2.0f - 50.0f, SDL_SCANCODE_W, SDL_SCANCODE_S);
        
        // Player 2 (Right) - AI Agent
        m_Player2 = std::make_unique<AgentPaddle>(width - 50.0f, height / 2.0f - 50.0f);

        // Ball (Center)
        m_Ball = std::make_unique<Ball>(width / 2.0f, height / 2.0f);
    }

    PongGame::~PongGame() {}

    void PongGame::Update(float deltaTime) {
        bool isEnterPressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_RETURN);
        bool isPausePressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_ESCAPE) || m_InputManager->IsKeyPressed(SDL_SCANCODE_P);

        if (m_State == GameState::StartMenu) {
            if (m_InputManager->IsKeyPressed(SDL_SCANCODE_1)) {
                m_Player2->SetDifficulty(AIDifficulty::Easy);
            } else if (m_InputManager->IsKeyPressed(SDL_SCANCODE_2)) {
                m_Player2->SetDifficulty(AIDifficulty::Medium);
            } else if (m_InputManager->IsKeyPressed(SDL_SCANCODE_3)) {
                m_Player2->SetDifficulty(AIDifficulty::Hard);
            }

            if (isEnterPressed && !m_WasEnterPressed) {
                m_State = GameState::Playing;
            }
        } 
        else if (m_State == GameState::Playing) {
            if (isPausePressed && !m_WasPausePressed) {
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
                ResetGame();
            } else if (m_Ball->IsOutOfBoundsRight(m_Width)) {
                m_Score1++;
                ResetGame();
            }
        } 
        else if (m_State == GameState::Paused) {
            if (isPausePressed && !m_WasPausePressed) {
                m_State = GameState::Playing;
            }
        }

        m_WasEnterPressed = isEnterPressed;
        m_WasPausePressed = isPausePressed;
    }

    void PongGame::Render() {
        // Clear screen to black
        m_Renderer->Clear(0, 0, 0, 255);

        if (m_State == GameState::StartMenu) {
            m_Renderer->DrawText("PONG", m_Width / 2 - 50, m_Height / 2 - 150, 255, 255, 255, 48);
            m_Renderer->DrawText("Press 1 for EASY AI", m_Width / 2 - 150, m_Height / 2 - 40, 0, 255, 0, 24);
            m_Renderer->DrawText("Press 2 for MEDIUM AI", m_Width / 2 - 150, m_Height / 2, 255, 255, 0, 24);
            m_Renderer->DrawText("Press 3 for HARD AI", m_Width / 2 - 150, m_Height / 2 + 40, 255, 0, 0, 24);
            
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
                // Dim the screen a bit using a semi-transparent black overlay
                // Note: SDL_SetRenderDrawBlendMode would need to be set to blend, 
                // but a simple text "PAUSED" works too.
                m_Renderer->DrawText("PAUSED", m_Width / 2 - 60, m_Height / 2 - 20, 255, 255, 0, 36);
                m_Renderer->DrawText("Press P or ESC to Resume", m_Width / 2 - 150, m_Height / 2 + 30, 255, 255, 255, 24);
            }
        }

        // Present to screen
        m_Renderer->Present();
    }

    void PongGame::CheckCollisions() {
        VECTOR::AABB ballAABB = m_Ball->GetAABB();
        VECTOR::AABB p1AABB = m_Player1->GetAABB();
        VECTOR::AABB p2AABB = m_Player2->GetAABB();

        if (ballAABB.Intersects(p1AABB)) {
            m_Ball->BounceX();
            // Prevent getting stuck by pushing the ball out
            m_Ball->SetPosition(p1AABB.x + p1AABB.w + 1.0f, ballAABB.y);
        } else if (ballAABB.Intersects(p2AABB)) {
            m_Ball->BounceX();
            m_Ball->SetPosition(p2AABB.x - ballAABB.w - 1.0f, ballAABB.y);
        }
    }

    void PongGame::ResetGame() {
        m_Player1->Reset(30.0f, m_Height / 2.0f - 50.0f);
        m_Player2->Reset(m_Width - 50.0f, m_Height / 2.0f - 50.0f);
        m_Ball->Reset(m_Width / 2.0f, m_Height / 2.0f);
    }

} // namespace Game
