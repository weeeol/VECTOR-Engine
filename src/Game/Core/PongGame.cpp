#include "Game/Core/PongGame.hpp"
#include "Game/Scenes/SplashScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Events/EventBus.hpp"
#include "Game/Events/GameEvents.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include "Engine/Core/Logger.hpp"
#include <imgui.h>

namespace Game {

    PongGame::PongGame(const std::string& title, int width, int height)
        : VECTOR::Application(title, width, height)
    {
    }

    PongGame::~PongGame() {}

    void PongGame::OnInit() {
        VECTOR_LOG_INFO("PongGame::OnInit() started");
        // Push initial scene
        auto initialScene = std::make_unique<SplashScene>(m_Width, m_Height, m_InputManager.get());
        VECTOR_LOG_INFO("Pushing SplashScene");
        VECTOR::SceneManager::Get().PushScene(std::move(initialScene));
        VECTOR_LOG_INFO("SplashScene Pushed");

        SetupEventSubscriptions();
    }

    void PongGame::SetupEventSubscriptions() {
        VECTOR::EventBus::Get().Subscribe<CollisionEvent>([](const CollisionEvent& event) {
            VECTOR::AudioManager::Get().PlaySound("assets/hit.wav");
        });
        
        VECTOR::EventBus::Get().Subscribe<ScoreEvent>([](const ScoreEvent& event) {
            // Can play a different sound here if desired
        });
    }

    void PongGame::Update(float deltaTime) {
        if (m_InputManager->IsKeyJustPressed(SDL_SCANCODE_F3)) {
            m_ShowDebugUI = !m_ShowDebugUI;
        }
        VECTOR::SceneManager::Get().Update(deltaTime);
    }

    void PongGame::Render() {
        m_Renderer->Clear(0, 0, 0, 255);
        VECTOR::SceneManager::Get().Render(m_Renderer.get());
    }

    void PongGame::OnImGuiRender() {
        if (m_ShowDebugUI) {
            ImGui::Begin("Engine Status");
            float fps = VECTOR::Application::Get().GetFPS();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", fps > 0.0f ? (1000.0f / fps) : 0.0f, fps);
            ImGui::End();
        }
    }

} // namespace Game
