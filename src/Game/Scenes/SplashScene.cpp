#include "Game/Scenes/SplashScene.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include "Game/Scenes/MainMenuScene.hpp"
#include "Game/Core/SaveManager.hpp"
#include <algorithm>
#include <cmath>

namespace Game {

// Phase timing (seconds)
static const float FADE_IN_DURATION = 0.8f;
static const float HOLD_DURATION = 1.6f;
static const float FADE_OUT_DURATION = 0.8f;
static const float PHASE_DURATION =
    FADE_IN_DURATION + HOLD_DURATION + FADE_OUT_DURATION; // 3.2s per phase

SplashScene::SplashScene(int width, int height,
                         VECTOR::InputManager *inputManager)
    : m_Width(width), m_Height(height), m_InputManager(inputManager),
      m_ParticleEmitter(200) {}

void SplashScene::OnEnter() {
  VECTOR_LOG_INFO("SplashScene::OnEnter()");
  m_Timer = 0.0f;
  m_Phase = 0;

  // Load and apply saved display settings so splash shows at correct resolution
  float volume = 0.5f;
  bool borderless = false;
  int resW = 1280, resH = 720;
  SaveManager::LoadSettings(volume, borderless, resW, resH);

  m_Width = resW;
  m_Height = resH;
  VECTOR::Application::Get().SetDimensions(resW, resH);
  VECTOR::Application::Get().GetRenderer()->SetResolution(resW, resH);
  VECTOR::AudioManager::Get().SetMusicVolume(volume);
  if (borderless) {
    VECTOR::Application::Get().GetRenderer()->SetBorderless(true);
  }
}

void SplashScene::TransitionToMenu() {
  auto menuScene = std::make_unique<MainMenuScene>(
      VECTOR::Application::Get().GetWidth(),
      VECTOR::Application::Get().GetHeight(), m_InputManager);
  VECTOR::SceneManager::Get().ChangeScene(std::move(menuScene));
}

void SplashScene::Update(float deltaTime) {
  m_Timer += deltaTime;

  // Subtle ambient particles
  if (rand() % 100 < 20) {
    float px = (float)(rand() % m_Width);
    float py = (float)(rand() % m_Height);
    m_ParticleEmitter.Emit(px, py, 1, 40, 80, 140, 10.0f, 2.5f);
  }
  m_ParticleEmitter.Update(deltaTime);

  // Skip on any key or mouse click
  if (m_Timer >
      0.3f) { // Small delay so input from launch doesn't skip immediately
    if (m_InputManager->IsKeyPressed(SDL_SCANCODE_SPACE) ||
        m_InputManager->IsKeyPressed(SDL_SCANCODE_RETURN) ||
        m_InputManager->IsKeyPressed(SDL_SCANCODE_ESCAPE) ||
        m_InputManager->IsMouseButtonJustPressed(SDL_BUTTON_LEFT)) {
      TransitionToMenu();
      return;
    }
  }

  // Check if current phase is done
  if (m_Timer >= PHASE_DURATION) {
    m_Timer = 0.0f;
    m_Phase++;
    if (m_Phase > 2) {
      TransitionToMenu();
      return;
    }
  }
}

void SplashScene::Render(VECTOR::Renderer *renderer) {
  // Calculate alpha for current phase (fade in → hold → fade out)
  float alpha = 0.0f;
  if (m_Timer < FADE_IN_DURATION) {
    // Fade in
    alpha = m_Timer / FADE_IN_DURATION;
  } else if (m_Timer < FADE_IN_DURATION + HOLD_DURATION) {
    // Hold at full
    alpha = 1.0f;
  } else {
    // Fade out
    float fadeProgress =
        (m_Timer - FADE_IN_DURATION - HOLD_DURATION) / FADE_OUT_DURATION;
    alpha = 1.0f - fadeProgress;
  }
  alpha = std::max(0.0f, std::min(1.0f, alpha));
  Uint8 a = (Uint8)(255.0f * alpha);

  // Particles behind everything
  m_ParticleEmitter.Render(renderer);

  // Determine what to show based on phase
  switch (m_Phase) {
  case 0: {
    // --- VECTOR Engine logo ---
    // Glow effect
    Uint8 glowA = (Uint8)(alpha * 80.0f);
    renderer->DrawText("VECTOR", m_Width / 2 - 110 - 3, m_Height / 2 - 35 - 3,
                       0, 140, 220, 48);
    renderer->DrawText("VECTOR", m_Width / 2 - 110 + 3, m_Height / 2 - 35 + 3,
                       0, 140, 220, 48);
    // Main text
    renderer->DrawText("VECTOR", m_Width / 2 - 110, m_Height / 2 - 35, a, a, a,
                       48);
    // "Engine" subtitle
    renderer->DrawText("E N G I N E", m_Width / 2 - 80, m_Height / 2 + 25,
                       (Uint8)(alpha * 0.7f * 255), (Uint8)(alpha * 0.7f * 255),
                       (Uint8)(alpha * 0.8f * 255), 20);
    // Decorative line
    Uint8 lineA = (Uint8)(alpha * 120.0f);
    int lineW = (int)(200.0f * alpha);
    renderer->DrawRect(m_Width / 2 - lineW / 2, m_Height / 2 + 15, lineW, 2, 0,
                       180, 255, lineA);
    break;
  }
  case 1: {
    // --- Created by ---
    renderer->DrawText("Created by", m_Width / 2 - 65, m_Height / 2 - 35,
                       (Uint8)(alpha * 0.6f * 255), (Uint8)(alpha * 0.6f * 255),
                       (Uint8)(alpha * 0.7f * 255), 20);
    // Author name with glow
    renderer->DrawText("Weeeol", m_Width / 2 - 55 - 2, m_Height / 2 + 2 - 2, 0,
                       140, 220, 36);
    renderer->DrawText("Weeeol", m_Width / 2 - 55 + 2, m_Height / 2 + 2 + 2, 0,
                       140, 220, 36);
    renderer->DrawText("Weeool", m_Width / 2 - 55, m_Height / 2 + 2, a, a, a,
                       36);
    break;
  }
  case 2: {
    // --- Game title ---
    // Big glow
    Uint8 glowA = (Uint8)(alpha * 60.0f);
    renderer->DrawText("PONG", m_Width / 2 - 80 - 5, m_Height / 2 - 30 - 5, 0,
                       180, 255, 64);
    renderer->DrawText("PONG", m_Width / 2 - 80 + 5, m_Height / 2 - 30 + 5, 0,
                       180, 255, 64);
    renderer->DrawText("PONG", m_Width / 2 - 80 - 3, m_Height / 2 - 30 + 3, 0,
                       180, 255, 64);
    renderer->DrawText("PONG", m_Width / 2 - 80 + 3, m_Height / 2 - 30 - 3, 0,
                       180, 255, 64);
    // Main title
    renderer->DrawText("PONG", m_Width / 2 - 80, m_Height / 2 - 30, a, a, a,
                       64);
    // Expanding decorative lines
    int lineW = (int)(300.0f * alpha);
    Uint8 lineA = (Uint8)(alpha * 100.0f);
    renderer->DrawRect(m_Width / 2 - lineW / 2, m_Height / 2 + 40, lineW, 2, 0,
                       180, 255, lineA);
    renderer->DrawRect(m_Width / 2 - lineW / 3, m_Height / 2 + 45,
                       lineW * 2 / 3, 1, 0, 180, 255, (Uint8)(lineA * 0.5f));
    break;
  }
  }

  // "Press any key to skip" hint (appears after a short delay)
  if (m_Timer > 1.0f || m_Phase > 0) {
    float pulse = 0.3f + 0.3f * std::sin(m_Timer * 3.0f);
    Uint8 hintA = (Uint8)(pulse * 255.0f);
    renderer->DrawText("Press any key to skip", m_Width / 2 - 120,
                       m_Height - 50, hintA, hintA, hintA, 16);
  }
}

} // namespace Game
