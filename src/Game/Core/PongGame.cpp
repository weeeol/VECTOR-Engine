#include "PongGame.hpp"
#include "Game/Scenes/MainMenuScene.hpp"
#include "Game/Scenes/GameplayScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Events/EventBus.hpp"
#include "Game/Events/GameEvents.hpp"
#include "Engine/Audio/AudioManager.hpp"

namespace Game {

    PongGame::PongGame(const std::string& title, int width, int height)
        : VECTOR::Application(title, width, height)
    {
    }

    PongGame::~PongGame() {}

    void PongGame::OnInit() {
        // Push initial scene
        auto firstScene = std::make_unique<MainMenuScene>(m_Width, m_Height, m_InputManager.get());
        VECTOR::SceneManager::Get().ChangeScene(std::move(firstScene));

        VECTOR::AudioManager::Get().PlayMusic("assets/bgm.wav");

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
        VECTOR::SceneManager::Get().Update(deltaTime);
    }

    void PongGame::Render() {
        m_Renderer->Clear(0, 0, 0, 255);
        
        VECTOR::SceneManager::Get().Render(m_Renderer.get());
        
        m_Renderer->Present();
    }

} // namespace Game
