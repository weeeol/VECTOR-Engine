#include "Game/Scenes/GameplayScene.hpp"
#include "Game/Scenes/MainMenuScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Events/EventBus.hpp"
#include "Game/Events/GameEvents.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/ECS/Components.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include "Engine/UI/UISlider.hpp"
#include "Engine/UI/UIButton.hpp"
#include "Game/Core/SaveManager.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Game {

    GameplayScene::GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty, GameMode mode)
        : m_Width(width), m_Height(height), m_InputManager(inputManager),
          m_Score1(0), m_Score2(0), m_IsPaused(false), m_WasPausePressed(false), m_DebugMode(false), m_WasF3Pressed(false),
          m_TrailEmitter(200), m_ExplosionEmitter(300), m_State(GameState::Playing), m_Mode(mode), m_Winner(0)
    {
        float volume = 0.5f;
        SaveManager::LoadData(m_HighScorePlayer, m_HighScoreAI, volume);

        m_SceneTexture = std::make_unique<VECTOR::Texture>(VECTOR::Application::Get().GetRenderer(), width, height);
        m_Registry.RegisterComponent<VECTOR::TransformComponent>();
        m_Registry.RegisterComponent<VECTOR::RigidBodyComponent>();
        m_Registry.RegisterComponent<VECTOR::RenderComponent>();
        m_Registry.RegisterComponent<PlayerInputComponent>();
        m_Registry.RegisterComponent<AIComponent>();
        m_Registry.RegisterComponent<BallComponent>();
        m_Registry.RegisterComponent<VECTOR::SpriteComponent>();

        auto physSys = std::make_unique<VECTOR::Box2DPhysicsSystem>();
        m_PhysicsSystem = physSys.get();
        m_Systems.push_back(std::move(physSys));

        auto ballSys = std::make_unique<BallMechanicsSystem>(m_Width, m_Height, &m_ExplosionEmitter);
        m_BallSystem = ballSys.get();
        m_Systems.push_back(std::move(ballSys));
        
        m_Systems.push_back(std::make_unique<PlayerInputSystem>(m_InputManager, m_Height));

        // Create Walls
        CreateBox(width/2.0f, -10.0f, width, 20.0f, b2_staticBody, 0.0f, 0.0f, 1.0f, false, (void*)0); // Top
        CreateBox(width/2.0f, height + 10.0f, width, 20.0f, b2_staticBody, 0.0f, 0.0f, 1.0f, false, (void*)0); // Bottom

        // Create Goals
        CreateBox(-10.0f, height/2.0f, 20.0f, height, b2_staticBody, 0.0f, 0.0f, 0.0f, true, (void*)2); // Left Goal
        CreateBox(width + 10.0f, height/2.0f, 20.0f, height, b2_staticBody, 0.0f, 0.0f, 0.0f, true, (void*)3); // Right Goal

        // Player 1
        m_Player1 = m_Registry.CreateEntity();
        m_Registry.AddComponent(m_Player1, VECTOR::TransformComponent{{30.0f, height / 2.0f - 50.0f}});
        m_Registry.AddComponent(m_Player1, VECTOR::RenderComponent{20.0f, 100.0f, 60, 200, 255, 255});
        m_Registry.AddComponent(m_Player1, PlayerInputComponent{SDL_SCANCODE_W, SDL_SCANCODE_S});
        b2BodyId p1Body = CreateBox(30.0f + 10.0f, height / 2.0f, 20.0f, 100.0f, b2_kinematicBody, 1.0f, 0.0f, 1.0f, false, (void*)4);
        m_Registry.AddComponent(m_Player1, VECTOR::RigidBodyComponent{p1Body});

        // Player 2
        m_Player2 = m_Registry.CreateEntity();
        m_Registry.AddComponent(m_Player2, VECTOR::TransformComponent{{width - 50.0f, height / 2.0f - 50.0f}});
        m_Registry.AddComponent(m_Player2, VECTOR::RenderComponent{20.0f, 100.0f, 255, 140, 50, 255});
        m_Registry.AddComponent(m_Player2, AIComponent{aiDifficulty, AIState::Idle, 0.0f, 0.2f, height / 2.0f});
        b2BodyId p2Body = CreateBox(width - 50.0f + 10.0f, height / 2.0f, 20.0f, 100.0f, b2_kinematicBody, 1.0f, 0.0f, 1.0f, false, (void*)4);
        m_Registry.AddComponent(m_Player2, VECTOR::RigidBodyComponent{p2Body});
        
        // Ball
        m_Ball = m_Registry.CreateEntity();
        m_Registry.AddComponent(m_Ball, VECTOR::TransformComponent{{width / 2.0f, height / 2.0f}});
        m_Registry.AddComponent(m_Ball, VECTOR::RenderComponent{15.0f, 15.0f, 255, 255, 255, 255});
        m_Registry.AddComponent(m_Ball, BallComponent{true});
        b2BodyId ballBody = CreateBox(width / 2.0f + 7.5f, height / 2.0f + 7.5f, 15.0f, 15.0f, b2_dynamicBody, 1.0f, 0.0f, 1.05f, false, (void*)1);
        m_Registry.AddComponent(m_Ball, VECTOR::RigidBodyComponent{ballBody});

        m_BallTexture = std::make_shared<VECTOR::Texture>(VECTOR::Application::Get().GetRenderer(), "s:/Projects/VECTOR-Engine/assets/ball_spritesheet.bmp");
        m_BallAnimator = std::make_shared<VECTOR::Animator>(m_BallTexture.get(), 32, 32, 4, 0.4f); // Slower animation
        m_BallAnimator->Play();
        m_Registry.AddComponent(m_Ball, VECTOR::SpriteComponent{m_BallAnimator.get()});

        m_Systems.push_back(std::make_unique<AISystem>(m_Ball, m_Height));

        auto volumeSlider = std::make_shared<VECTOR::UISlider>(width / 2 - 100, height / 2 + 20, 200, 20, volume, [](float val) {
            VECTOR::AudioManager::Get().SetMusicVolume(val);
            // Save on change is tricky here, but we can do it on exit
        });
        m_PauseMenuUI.AddElement(volumeSlider);
        
        auto mainMenuButton = std::make_shared<VECTOR::UIButton>(width / 2 - 100, height / 2 + 60, 200, 40, "Main Menu", [this, volumeSlider]() {
            SaveManager::SaveData(m_HighScorePlayer, m_HighScoreAI, volumeSlider->GetValue());
            auto menuScene = std::make_unique<MainMenuScene>(m_Width, m_Height, m_InputManager);
            VECTOR::SceneManager::Get().ChangeScene(std::move(menuScene));
        });
        m_PauseMenuUI.AddElement(mainMenuButton);

        VECTOR::AudioManager::Get().SetMusicVolume(volume);
    }

    GameplayScene::~GameplayScene() {}

    b2BodyId GameplayScene::CreateBox(float x, float y, float width, float height, b2BodyType type, float density, float friction, float restitution, bool isSensor, void* userData) {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = type;
        bodyDef.position = b2Vec2{x / VECTOR::PIXELS_PER_METER, y / VECTOR::PIXELS_PER_METER};
        bodyDef.userData = userData;
        bodyDef.fixedRotation = true;
        bodyDef.isBullet = (type == b2_dynamicBody);
        b2BodyId bodyId = b2CreateBody(m_PhysicsSystem->GetWorld(), &bodyDef);

        b2Polygon box = b2MakeBox((width / 2.0f) / VECTOR::PIXELS_PER_METER, (height / 2.0f) / VECTOR::PIXELS_PER_METER);

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = density;
        shapeDef.material.friction = friction;
        shapeDef.material.restitution = restitution;
        shapeDef.isSensor = isSensor;
        shapeDef.enableContactEvents = true; // Enables events when it touches other bodies
        shapeDef.enableSensorEvents = true;

        b2CreatePolygonShape(bodyId, &shapeDef, &box);
        return bodyId;
    }

    void GameplayScene::OnEnter() {
        ResetGame();
        m_WasPausePressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_P);
    }

    void GameplayScene::Update(float deltaTime) {
        bool isPausePressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_P) || m_InputManager->IsKeyPressed(SDL_SCANCODE_ESCAPE);
        bool isF3Pressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_F3);

        if (isF3Pressed && !m_WasF3Pressed) m_DebugMode = !m_DebugMode;
        m_WasF3Pressed = isF3Pressed;

        if (m_State == GameState::GameOver) {
            m_GameOverTimer += deltaTime;
            // Check for space or escape to return to main menu
            if (m_InputManager->IsKeyPressed(SDL_SCANCODE_SPACE) || m_InputManager->IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
                auto menuScene = std::make_unique<MainMenuScene>(m_Width, m_Height, m_InputManager);
                VECTOR::SceneManager::Get().ChangeScene(std::move(menuScene));
            }
            return;
        }

        if (isPausePressed && !m_WasPausePressed) m_IsPaused = !m_IsPaused;
        m_WasPausePressed = isPausePressed;

        if (m_IsPaused) {
            m_PauseMenuUI.Update(m_InputManager, deltaTime);
        } else {
            if (m_ShakeTimer > 0.0f) {
                m_ShakeTimer -= deltaTime;
            }
            m_BackgroundOffset += 50.0f * deltaTime;
            if (m_BackgroundOffset > 60.0f) m_BackgroundOffset -= 60.0f;

            for (auto& system : m_Systems) system->Update(m_Registry, deltaTime);
            
            // Check Box2D 3 Contact Events
            b2ContactEvents contactEvents = b2World_GetContactEvents(m_PhysicsSystem->GetWorld());
            for (int i = 0; i < contactEvents.beginCount; ++i) {
                b2ContactBeginTouchEvent* event = &contactEvents.beginEvents[i];
                b2BodyId bodyA = b2Shape_GetBody(event->shapeIdA);
                b2BodyId bodyB = b2Shape_GetBody(event->shapeIdB);
                
                intptr_t dataA = (intptr_t)b2Body_GetUserData(bodyA);
                intptr_t dataB = (intptr_t)b2Body_GetUserData(bodyB);
                
                if ((dataA == 1 && dataB == 4) || (dataA == 4 && dataB == 1)) {
                    b2BodyId ballBody = (dataA == 1) ? bodyA : bodyB;
                    b2Vec2 pos = b2Body_GetPosition(ballBody);
                    m_ExplosionEmitter.Emit(pos.x * VECTOR::PIXELS_PER_METER, pos.y * VECTOR::PIXELS_PER_METER, 30, 255, 100, 50, 300.0f, 0.5f);
                    VECTOR::EventBus::Get().Publish<CollisionEvent>();
                }
            }
            
            // Check Box2D 3 Sensor Events
            b2SensorEvents sensorEvents = b2World_GetSensorEvents(m_PhysicsSystem->GetWorld());
            for (int i = 0; i < sensorEvents.beginCount; ++i) {
                b2SensorBeginTouchEvent* event = &sensorEvents.beginEvents[i];
                b2BodyId visitorBody = b2Shape_GetBody(event->visitorShapeId);
                b2BodyId sensorBody = b2Shape_GetBody(event->sensorShapeId);
                
                intptr_t dataVisitor = (intptr_t)b2Body_GetUserData(visitorBody);
                intptr_t dataSensor = (intptr_t)b2Body_GetUserData(sensorBody);
                
                if (dataVisitor == 1) { // Ball entered sensor
                    if (dataSensor == 2) m_BallSystem->SetRightScored();
                    if (dataSensor == 3) m_BallSystem->SetLeftScored();
                }
            }

            auto& ballRB = m_Registry.GetComponent<VECTOR::RigidBodyComponent>(m_Ball);
            b2Vec2 pos = b2Body_GetPosition(ballRB.bodyId);
            m_TrailEmitter.Emit(pos.x * VECTOR::PIXELS_PER_METER, pos.y * VECTOR::PIXELS_PER_METER, 1, 255, 200, 50, 20.0f, 0.3f);
            
            m_TrailEmitter.Update(deltaTime);
            m_ExplosionEmitter.Update(deltaTime);
            m_BallAnimator->Update(deltaTime);

            if (m_BallSystem->WasLeftScored()) {
                m_Score1++;
                VECTOR::EventBus::Get().Publish<ScoreEvent>(1);
                
                if (m_Mode == GameMode::FivePoint && m_Score1 >= WINNING_SCORE) { 
                    m_State = GameState::GameOver; 
                    m_Winner = 1; 
                    SaveManager::SaveData(std::max(m_HighScorePlayer, m_Score1), m_HighScoreAI, VECTOR::AudioManager::Get().GetMusicVolume()); 
                } else {
                    m_ShakeTimer = 0.3f; m_ShakeMagnitude = 15.0f; // Shake on normal goals
                    ResetGame();
                }
                
                m_BallSystem->ResetScoreFlags();
            } else if (m_BallSystem->WasRightScored()) {
                m_Score2++;
                VECTOR::EventBus::Get().Publish<ScoreEvent>(2);
                
                if (m_Mode == GameMode::FivePoint && m_Score2 >= WINNING_SCORE) { 
                    m_State = GameState::GameOver; 
                    m_Winner = 2; 
                    SaveManager::SaveData(m_HighScorePlayer, std::max(m_HighScoreAI, m_Score2), VECTOR::AudioManager::Get().GetMusicVolume()); 
                } else {
                    m_ShakeTimer = 0.3f; m_ShakeMagnitude = 15.0f; // Shake on normal goals
                    ResetGame();
                }
                
                m_BallSystem->ResetScoreFlags();
            }
            
            // Update high score continuously
            m_HighScorePlayer = std::max(m_HighScorePlayer, m_Score1);
            m_HighScoreAI = std::max(m_HighScoreAI, m_Score2);
            
            m_Registry.View<VECTOR::TransformComponent, VECTOR::RigidBodyComponent, VECTOR::RenderComponent>([&](VECTOR::Entity entity) {
                auto& t = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
                auto& r = m_Registry.GetComponent<VECTOR::RenderComponent>(entity);
                t.position.x -= r.width / 2.0f;
                t.position.y -= r.height / 2.0f;
            });
        }
    }

    void GameplayScene::Render(VECTOR::Renderer* renderer) {
        // Post-Processing: Set Render Target
        renderer->SetRenderTarget(m_SceneTexture.get());
        renderer->Clear(0, 0, 0, 255);

        // Screen Shake
        int offsetX = 0;
        int offsetY = 0;
        if (m_ShakeTimer > 0.0f) {
            offsetX = (rand() % (int)m_ShakeMagnitude * 2) - (int)m_ShakeMagnitude;
            offsetY = (rand() % (int)m_ShakeMagnitude * 2) - (int)m_ShakeMagnitude;
        }

        // Draw Dynamic Background Grid
        for (int x = (int)m_BackgroundOffset; x < m_Width; x += 60) {
            renderer->DrawRect(x + offsetX, 0 + offsetY, 1, m_Height, 15, 15, 40, 70);
        }
        for (int y = (int)m_BackgroundOffset; y < m_Height; y += 60) {
            renderer->DrawRect(0 + offsetX, y + offsetY, m_Width, 1, 15, 15, 40, 70);
        }

        m_TrailEmitter.Render(renderer, offsetX, offsetY);
        m_ExplosionEmitter.Render(renderer, offsetX, offsetY);
        
        m_Registry.View<VECTOR::TransformComponent, VECTOR::RenderComponent>([&](VECTOR::Entity entity) {
            auto& transform = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& r = m_Registry.GetComponent<VECTOR::RenderComponent>(entity);
            if (m_Registry.HasComponent<VECTOR::SpriteComponent>(entity)) {
                auto& sprite = m_Registry.GetComponent<VECTOR::SpriteComponent>(entity);
                auto& rb = m_Registry.GetComponent<VECTOR::RigidBodyComponent>(entity);
                
                int drawWidth = 48; 
                int drawHeight = 48;
                
                // Squish and Stretch based on velocity
                b2Vec2 vel = b2Body_GetLinearVelocity(rb.bodyId);
                float speed = b2Length(vel);
                if (speed > 5.0f) {
                    if (std::abs(vel.x) > std::abs(vel.y)) {
                        drawWidth += (int)(speed * 1.5f);
                        drawHeight -= (int)(speed * 0.5f);
                    } else {
                        drawHeight += (int)(speed * 1.5f);
                        drawWidth -= (int)(speed * 0.5f);
                    }
                }

                int drawX = (int)transform.position.x - (drawWidth - (int)r.width) / 2 + offsetX;
                int drawY = (int)transform.position.y - (drawHeight - (int)r.height) / 2 + offsetY;
                sprite.animator->Render(renderer, drawX, drawY, drawWidth, drawHeight);
            } else {
                // Draw paddle glow (larger, low-alpha rect behind the paddle)
                int px = (int)transform.position.x + offsetX;
                int py = (int)transform.position.y + offsetY;
                int pw = (int)r.width;
                int ph = (int)r.height;
                renderer->DrawRect(px - 4, py - 4, pw + 8, ph + 8, r.r, r.g, r.b, 40);
                renderer->DrawRect(px - 2, py - 2, pw + 4, ph + 4, r.r, r.g, r.b, 80);
                // Draw the paddle itself
                renderer->DrawRect(px, py, pw, ph, r.r, r.g, r.b, r.a);
            }
        });

        // --- Center line with glow ---
        // Glow layer (wider, lower alpha)
        for (int y = 0; y < m_Height; y += 30) {
            renderer->DrawRect(m_Width / 2 - 4 + offsetX, y + offsetY, 8, 15, 255, 255, 255, 30);
        }
        // Main dashed line
        for (int y = 0; y < m_Height; y += 30) {
            renderer->DrawRect(m_Width / 2 - 2 + offsetX, y + offsetY, 4, 15, 255, 255, 255, 120);
        }

        // --- Scores with glow ---
        std::string scoreStr1 = std::to_string(m_Score1);
        std::string scoreStr2 = std::to_string(m_Score2);
        int s1x = m_Width / 4 - 15 + offsetX;
        int s2x = 3 * m_Width / 4 - 15 + offsetX;
        int sy = 30 + offsetY;
        // Glow layers (cyan for P1, orange for P2)
        renderer->DrawText(scoreStr1, s1x - 2, sy - 2, 60, 200, 255, 48);
        renderer->DrawText(scoreStr1, s1x + 2, sy + 2, 60, 200, 255, 48);
        renderer->DrawText(scoreStr1, s1x, sy, 255, 255, 255, 48);
        renderer->DrawText(scoreStr2, s2x - 2, sy - 2, 255, 140, 50, 48);
        renderer->DrawText(scoreStr2, s2x + 2, sy + 2, 255, 140, 50, 48);
        renderer->DrawText(scoreStr2, s2x, sy, 255, 255, 255, 48);

        // --- Game Over screen ---
        if (m_State == GameState::GameOver) {
            // Dark overlay
            renderer->DrawRect(0, 0, m_Width, m_Height, 0, 0, 0, 180);

            std::string winText = (m_Winner == 1) ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
            int wtx = m_Width / 2 - 140 + offsetX;
            int wty = m_Height / 2 - 60 + offsetY;

            // Gold glow behind winner text
            renderer->DrawRect(wtx - 20, wty - 10, 320, 50, 255, 200, 0, 25);
            renderer->DrawRect(wtx - 10, wty - 5, 300, 40, 255, 200, 0, 15);

            // Flickering win text
            float flicker = 0.7f + 0.3f * std::sin(m_GameOverTimer * 4.0f);
            Uint8 flickerAlpha = (Uint8)(255.0f * flicker);
            // Glow layers
            renderer->DrawText(winText, wtx - 2, wty - 2, 255, 180, 0, 48);
            renderer->DrawText(winText, wtx + 2, wty + 2, 255, 180, 0, 48);
            // Main text
            renderer->DrawText(winText, wtx, wty, 255, 215, 0, 48);

            // Final score
            std::string finalScore = std::to_string(m_Score1) + " - " + std::to_string(m_Score2);
            renderer->DrawText(finalScore, m_Width / 2 - 40 + offsetX, m_Height / 2 + 5 + offsetY, 220, 220, 220, 32);

            // Return hint
            float hintPulse = 0.5f + 0.5f * std::sin(m_GameOverTimer * 2.5f);
            Uint8 hintAlpha = (Uint8)(200.0f * hintPulse);
            renderer->DrawText("Press SPACE to return", m_Width / 2 - 130 + offsetX, m_Height / 2 + 50 + offsetY, hintAlpha, hintAlpha, hintAlpha, 24);
        } else if (m_IsPaused) {
            // Translucent overlay
            renderer->DrawRect(0, 0, m_Width, m_Height, 0, 0, 0, 200);

            // Panel with border
            int panelX = m_Width / 2 - 160;
            int panelY = m_Height / 2 - 110;
            int panelW = 320;
            int panelH = 270;
            // Outer glow border
            renderer->DrawRect(panelX - 2, panelY - 2, panelW + 4, panelH + 4, 0, 140, 200, 60);
            // Panel background
            renderer->DrawRect(panelX, panelY, panelW, panelH, 25, 25, 35, 240);

            // Title
            renderer->DrawText("PAUSED", m_Width / 2 - 60, panelY + 15, 255, 255, 255, 48);
            // Underline separator
            renderer->DrawRect(panelX + 30, panelY + 65, panelW - 60, 2, 0, 180, 255, 100);
            renderer->DrawRect(panelX + 60, panelY + 69, panelW - 120, 1, 0, 180, 255, 40);

            renderer->DrawText("Volume", m_Width / 2 - 35, m_Height / 2 - 10, 200, 200, 220, 24);
            
            m_PauseMenuUI.Render(renderer);

            // Resume hint
            renderer->DrawText("Press P to Resume", m_Width / 2 - 105, panelY + panelH - 35, 120, 120, 140, 20);
        }

        // Post-Processing: Render Target to Screen
        renderer->ResetRenderTarget();
        renderer->Clear(0, 0, 0, 255);
        renderer->DrawTexture(m_SceneTexture.get(), 0, 0, m_Width, m_Height);

        // CRT Scanline Effect (draw semi-transparent black lines)
        renderer->SetRenderDrawBlendMode(SDL_BLENDMODE_BLEND);
        for (int y = 0; y < m_Height; y += 4) {
            renderer->DrawRect(0, y, m_Width, 2, 0, 0, 0, 50);
        }

        // High Score Overlay
        renderer->DrawText("Best: " + std::to_string(m_HighScorePlayer), 10, 10, 100, 100, 120, 20);
        renderer->DrawText("AI Best: " + std::to_string(m_HighScoreAI), m_Width - 180, 10, 100, 100, 120, 20);
    }

    void GameplayScene::ResetGame() {
        auto& p1RB = m_Registry.GetComponent<VECTOR::RigidBodyComponent>(m_Player1);
        auto& p2RB = m_Registry.GetComponent<VECTOR::RigidBodyComponent>(m_Player2);
        auto& bRB = m_Registry.GetComponent<VECTOR::RigidBodyComponent>(m_Ball);

        b2Body_SetTransform(p1RB.bodyId, b2Vec2{(30.0f + 10.0f) / VECTOR::PIXELS_PER_METER, (m_Height / 2.0f) / VECTOR::PIXELS_PER_METER}, b2Rot_identity);
        b2Body_SetTransform(p2RB.bodyId, b2Vec2{(m_Width - 50.0f + 10.0f) / VECTOR::PIXELS_PER_METER, (m_Height / 2.0f) / VECTOR::PIXELS_PER_METER}, b2Rot_identity);
        b2Body_SetTransform(bRB.bodyId, b2Vec2{(m_Width / 2.0f + 7.5f) / VECTOR::PIXELS_PER_METER, (m_Height / 2.0f + 7.5f) / VECTOR::PIXELS_PER_METER}, b2Rot_identity);
        
        float dirX = (rand() % 2 == 0) ? -1.0f : 1.0f;
        float dirY = (rand() % 2 == 0) ? -1.0f : 1.0f;
        b2Body_SetLinearVelocity(bRB.bodyId, b2Vec2{dirX * 8.0f, dirY * 8.0f});
    }

}
