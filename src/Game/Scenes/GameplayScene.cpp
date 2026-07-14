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
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Game {

    GameplayScene::GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty)
        : m_Width(width), m_Height(height), m_InputManager(inputManager),
          m_Score1(0), m_Score2(0), m_IsPaused(false), m_WasPausePressed(false), m_DebugMode(false), m_WasF3Pressed(false),
          m_TrailEmitter(200), m_ExplosionEmitter(300), m_State(GameState::Playing), m_Winner(0)
    {
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
        m_Registry.AddComponent(m_Player1, VECTOR::RenderComponent{20.0f, 100.0f, 255, 255, 255, 255});
        m_Registry.AddComponent(m_Player1, PlayerInputComponent{SDL_SCANCODE_W, SDL_SCANCODE_S});
        b2BodyId p1Body = CreateBox(30.0f + 10.0f, height / 2.0f, 20.0f, 100.0f, b2_kinematicBody, 1.0f, 0.0f, 1.0f, false, (void*)4);
        m_Registry.AddComponent(m_Player1, VECTOR::RigidBodyComponent{p1Body});

        // Player 2
        m_Player2 = m_Registry.CreateEntity();
        m_Registry.AddComponent(m_Player2, VECTOR::TransformComponent{{width - 50.0f, height / 2.0f - 50.0f}});
        m_Registry.AddComponent(m_Player2, VECTOR::RenderComponent{20.0f, 100.0f, 255, 255, 255, 255});
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

        auto volumeSlider = std::make_shared<VECTOR::UISlider>(width / 2 - 100, height / 2 + 20, 200, 20, 0.5f, [](float val) {
            VECTOR::AudioManager::Get().SetMusicVolume(val);
        });
        m_PauseMenuUI.AddElement(volumeSlider);
        
        auto mainMenuButton = std::make_shared<VECTOR::UIButton>(width / 2 - 100, height / 2 + 60, 200, 40, "Main Menu", [this]() {
            auto menuScene = std::make_unique<MainMenuScene>(m_Width, m_Height, m_InputManager);
            VECTOR::SceneManager::Get().ChangeScene(std::move(menuScene));
        });
        m_PauseMenuUI.AddElement(mainMenuButton);

        VECTOR::AudioManager::Get().SetMusicVolume(0.5f);
    }

    GameplayScene::~GameplayScene() {}

    b2BodyId GameplayScene::CreateBox(float x, float y, float width, float height, b2BodyType type, float density, float friction, float restitution, bool isSensor, void* userData) {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = type;
        bodyDef.position = (b2Vec2){x / VECTOR::PIXELS_PER_METER, y / VECTOR::PIXELS_PER_METER};
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

        if (m_State == GameState::GameOver) return;

        if (isPausePressed && !m_WasPausePressed) m_IsPaused = !m_IsPaused;
        m_WasPausePressed = isPausePressed;

        if (m_IsPaused) {
            m_PauseMenuUI.Update(m_InputManager, deltaTime);
        } else {
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
                if (m_Score1 >= WINNING_SCORE) { m_State = GameState::GameOver; m_Winner = 1; } 
                else ResetGame();
                m_BallSystem->ResetScoreFlags();
            } else if (m_BallSystem->WasRightScored()) {
                m_Score2++;
                VECTOR::EventBus::Get().Publish<ScoreEvent>(2);
                if (m_Score2 >= WINNING_SCORE) { m_State = GameState::GameOver; m_Winner = 2; } 
                else ResetGame();
                m_BallSystem->ResetScoreFlags();
            }
            
            m_Registry.View<VECTOR::TransformComponent, VECTOR::RigidBodyComponent, VECTOR::RenderComponent>([&](VECTOR::Entity entity) {
                auto& t = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
                auto& r = m_Registry.GetComponent<VECTOR::RenderComponent>(entity);
                t.position.x -= r.width / 2.0f;
                t.position.y -= r.height / 2.0f;
            });
        }
    }

    void GameplayScene::Render(VECTOR::Renderer* renderer) {
        m_TrailEmitter.Render(renderer);
        m_ExplosionEmitter.Render(renderer);
        
        m_Registry.View<VECTOR::TransformComponent, VECTOR::RenderComponent>([&](VECTOR::Entity entity) {
            auto& transform = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& r = m_Registry.GetComponent<VECTOR::RenderComponent>(entity);
            if (m_Registry.HasComponent<VECTOR::SpriteComponent>(entity)) {
                auto& sprite = m_Registry.GetComponent<VECTOR::SpriteComponent>(entity);
                // Draw sprite scaled up so it is easier to see, centered on the physical box
                int drawWidth = 48; // Draw it 3x larger than the 15x15 hitbox
                int drawHeight = 48;
                int drawX = (int)transform.position.x - (drawWidth - (int)r.width) / 2;
                int drawY = (int)transform.position.y - (drawHeight - (int)r.height) / 2;
                sprite.animator->Render(renderer, drawX, drawY, drawWidth, drawHeight);
            } else {
                renderer->DrawRect((int)transform.position.x, (int)transform.position.y, (int)r.width, (int)r.height, r.r, r.g, r.b, r.a);
            }
        });

        for (int y = 0; y < m_Height; y += 30) renderer->DrawRect(m_Width / 2 - 2, y, 4, 15, 255, 255, 255, 100);
        renderer->DrawText(std::to_string(m_Score1), m_Width / 2 - 50, 20, 255, 255, 255, 48);
        renderer->DrawText(std::to_string(m_Score2), m_Width / 2 + 20, 20, 255, 255, 255, 48);

        if (m_State == GameState::GameOver) {
            std::string winText = (m_Winner == 1) ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
            renderer->DrawText(winText, m_Width / 2 - 140, m_Height / 2 - 50, 255, 215, 0, 48);
        } else if (m_IsPaused) {
            renderer->DrawRect(0, 0, m_Width, m_Height, 0, 0, 0, 200); // Translucent overlay
            
            // Draw a panel for the pause menu
            renderer->DrawRect(m_Width / 2 - 150, m_Height / 2 - 100, 300, 250, 40, 40, 50, 230); // Panel
            
            renderer->DrawText("PAUSED", m_Width / 2 - 60, m_Height / 2 - 80, 255, 255, 255, 48);
            renderer->DrawText("Volume", m_Width / 2 - 40, m_Height / 2 - 10, 200, 200, 200, 24);
            
            m_PauseMenuUI.Render(renderer);
        }
    }

    void GameplayScene::ResetGame() {
        auto& p1RB = m_Registry.GetComponent<VECTOR::RigidBodyComponent>(m_Player1);
        auto& p2RB = m_Registry.GetComponent<VECTOR::RigidBodyComponent>(m_Player2);
        auto& bRB = m_Registry.GetComponent<VECTOR::RigidBodyComponent>(m_Ball);

        b2Body_SetTransform(p1RB.bodyId, (b2Vec2){(30.0f + 10.0f) / VECTOR::PIXELS_PER_METER, (m_Height / 2.0f) / VECTOR::PIXELS_PER_METER}, b2Rot_identity);
        b2Body_SetTransform(p2RB.bodyId, (b2Vec2){(m_Width - 50.0f + 10.0f) / VECTOR::PIXELS_PER_METER, (m_Height / 2.0f) / VECTOR::PIXELS_PER_METER}, b2Rot_identity);
        b2Body_SetTransform(bRB.bodyId, (b2Vec2){(m_Width / 2.0f + 7.5f) / VECTOR::PIXELS_PER_METER, (m_Height / 2.0f + 7.5f) / VECTOR::PIXELS_PER_METER}, b2Rot_identity);
        
        float dirX = (rand() % 2 == 0) ? -1.0f : 1.0f;
        float dirY = (rand() % 2 == 0) ? -1.0f : 1.0f;
        b2Body_SetLinearVelocity(bRB.bodyId, (b2Vec2){dirX * 8.0f, dirY * 8.0f});
    }

}
