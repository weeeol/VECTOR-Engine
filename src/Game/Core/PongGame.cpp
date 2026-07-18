#include "Game/Core/PongGame.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Events/EventBus.hpp"
#include "Game/Events/GameEvents.hpp"
#include "Game/Scenes/GameplayScene.hpp"
#include "Game/Scenes/MainMenuScene.hpp"


namespace Game {

PongGame::PongGame(const std::string &title, int width, int height)
    : VECTOR::Application(title, width, height) {}

PongGame::~PongGame() {}

void PongGame::OnInit() {
  VECTOR_LOG_INFO("PongGame::OnInit() started");

  // Push initial scene
  auto firstScene =
      std::make_unique<MainMenuScene>(m_Width, m_Height, m_InputManager.get());
  VECTOR_LOG_INFO("MainMenuScene created");

  VECTOR::SceneManager::Get().ChangeScene(std::move(firstScene));
  VECTOR_LOG_INFO("MainMenuScene pushed to SceneManager");

  VECTOR::AudioManager::Get().PlayMusic("assets/bgm.wav");
  VECTOR_LOG_INFO("BGM playback started");

  SetupEventSubscriptions();
  VECTOR_LOG_INFO("PongGame::OnInit() finished");
}

void PongGame::SetupEventSubscriptions() {
  VECTOR::EventBus::Get().Subscribe<CollisionEvent>(
      [](const CollisionEvent &event) {
        VECTOR::AudioManager::Get().PlaySound("assets/hit.wav");
      });

  VECTOR::EventBus::Get().Subscribe<ScoreEvent>([](const ScoreEvent &event) {
    // Can play a different sound here if desired
  });
}

void PongGame::Update(float deltaTime) {
  VECTOR::SceneManager::Get().Update(deltaTime);
}

void PongGame::Render() {
  m_Renderer->Clear(0, 0, 0, 255);

  m_Renderer->BeginImGuiFrame();

  VECTOR::SceneManager::Get().Render(m_Renderer.get());

  m_Renderer->EndImGuiFrame();

  m_Renderer->Present();
}

} // namespace Game

VECTOR::Application *VECTOR::CreateApplication() {
  return new Game::PongGame("VECTOR Engine 3D", 1920, 1080);
}
