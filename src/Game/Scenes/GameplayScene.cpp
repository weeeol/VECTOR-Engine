#include "Game/Scenes/GameplayScene.hpp"
#include "Game/Scenes/MainMenuScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/Material.hpp"
#include "Engine/Events/EventBus.hpp"
#include "Game/Events/GameEvents.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include "Engine/ECS/Components.hpp"
#include "Engine/ECS/UIComponents.hpp"
#include <glm/gtc/matrix_transform.hpp>
// #include <GL/glew.h> Removed
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Shader.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include <SDL3/SDL.h>
#include <imgui.h>

namespace Game {

    GameplayScene::GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty)
        : m_Width(width), m_Height(height), m_InputManager(inputManager)
    {
        m_Registry.RegisterComponent<VECTOR::TransformComponent>();
        m_Registry.RegisterComponent<VECTOR::RigidBodyComponent>();
        m_Registry.RegisterComponent<VECTOR::RenderComponent>();
        m_Registry.RegisterComponent<VECTOR::MeshComponent>();
        m_Registry.RegisterComponent<VECTOR::CameraComponent>();
        m_Registry.RegisterComponent<VECTOR::PointLightComponent>();
        m_Registry.RegisterComponent<VECTOR::DirectionalLightComponent>();
        m_Registry.RegisterComponent<AIComponent>(); // Reusing AIComponent for enemies
        m_Registry.RegisterComponent<VECTOR::TagComponent>();

        m_EditorUI = std::make_unique<EditorUI>(m_Registry, VECTOR::Application::Get().GetRenderer());

        auto physSys = std::make_unique<VECTOR::BulletPhysicsSystem>();
        m_PhysicsSystem = physSys.get();
        m_Systems.push_back(std::move(physSys));

        auto camSys = std::make_unique<CameraSystem>(m_InputManager);
        m_CameraSystem = camSys.get();
        m_Systems.push_back(std::move(camSys));

        m_Systems.push_back(std::make_unique<ShootingSystem>(m_InputManager, m_PhysicsSystem));

        m_CubeMesh = VECTOR::Mesh::CreateCube();

        // Create unlit material for objects like the sun
        m_UnlitMaterial = std::make_shared<VECTOR::Material>();
        m_UnlitMaterial->shader = VECTOR::ResourceManager::Get().GetShader("Default3D");
        m_UnlitMaterial->isUnlit = true;
        m_UnlitMaterial->albedoColor = glm::vec4(1.0f, 1.0f, 0.8f, 1.0f);

        // UI Registry Setup
        m_UIRegistry.RegisterComponent<VECTOR::UIRectComponent>();
        m_UIRegistry.RegisterComponent<VECTOR::UITextComponent>();
        m_UIRegistry.RegisterComponent<VECTOR::UIButtonComponent>();
        m_UIRegistry.RegisterComponent<VECTOR::UISliderComponent>();

        m_UISystem = std::make_unique<VECTOR::UISystem>(m_InputManager);

        CreateUI();

        GenerateArena();

        m_InputManager->SetRelativeMouseMode(true);
    }

    void GameplayScene::ClearUI() {
        m_UIRegistry.Clear();
    }

    void GameplayScene::OnResize(int width, int height) {
        m_Width = width;
        m_Height = height;
        CreateUI();
    }

    void GameplayScene::CreateUI() {
        ClearUI();
        
        int btnWidth = 200;
        int btnHeight = 40;
        int startX = m_Width / 2 - btnWidth / 2;
        int startY = m_Height / 2 - 20;

        if (m_State == GameState::Paused) {
            VECTOR::Entity resumeBtn = m_UIRegistry.CreateEntity();
            m_UIRegistry.AddComponent(resumeBtn, VECTOR::UIRectComponent(startX, startY - 50, btnWidth, btnHeight, glm::vec4(50/255.0f, 100/255.0f, 50/255.0f, 1.0f)));
            m_UIRegistry.AddComponent(resumeBtn, VECTOR::UITextComponent("Resume", 24, glm::vec4(1.0f)));
            auto& textR = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(resumeBtn);
            textR.offsetX = (btnWidth / 2) - (std::string("Resume").length() * 6);
            textR.offsetY = (btnHeight / 2) - 12;
            m_UIRegistry.AddComponent(resumeBtn, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 100/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 200/255.0f, 100/255.0f, 1.0f),
                [this]() {
                    m_State = GameState::Playing;
                    m_InputManager->SetRelativeMouseMode(true);
                }
            ));

            VECTOR::Entity settingsBtn = m_UIRegistry.CreateEntity();
            m_UIRegistry.AddComponent(settingsBtn, VECTOR::UIRectComponent(startX, startY + 10, btnWidth, btnHeight, glm::vec4(50/255.0f, 50/255.0f, 100/255.0f, 1.0f)));
            m_UIRegistry.AddComponent(settingsBtn, VECTOR::UITextComponent("Settings", 24, glm::vec4(1.0f)));
            auto& sText = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(settingsBtn);
            sText.offsetX = (btnWidth / 2) - (std::string("Settings").length() * 6);
            sText.offsetY = (btnHeight / 2) - 12;
            m_UIRegistry.AddComponent(settingsBtn, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 50/255.0f, 100/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 200/255.0f, 1.0f),
                [this]() {
                    m_State = GameState::Settings;
                    m_NeedsUIRefresh = true;
                }
            ));

            VECTOR::Entity mainMenuBtn = m_UIRegistry.CreateEntity();
            m_UIRegistry.AddComponent(mainMenuBtn, VECTOR::UIRectComponent(startX, startY + 70, btnWidth, btnHeight, glm::vec4(100/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_UIRegistry.AddComponent(mainMenuBtn, VECTOR::UITextComponent("Main Menu", 24, glm::vec4(1.0f)));
            auto& textC = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(mainMenuBtn);
            textC.offsetX = (btnWidth / 2) - (std::string("Main Menu").length() * 6);
            textC.offsetY = (btnHeight / 2) - 12;
            m_UIRegistry.AddComponent(mainMenuBtn, VECTOR::UIButtonComponent(
                glm::vec4(100/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(200/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                [this]() {
                    auto menuScene = std::make_unique<MainMenuScene>(m_Width, m_Height, m_InputManager);
                    VECTOR::SceneManager::Get().ChangeScene(std::move(menuScene));
                }
            ));

            VECTOR::Entity volSlider = m_UIRegistry.CreateEntity();
            m_UIRegistry.AddComponent(volSlider, VECTOR::UIRectComponent(m_Width - 250, 50, 200, 20, glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f)));
            m_UIRegistry.AddComponent(volSlider, VECTOR::UISliderComponent(VECTOR::AudioManager::Get().GetMusicVolume(), [](float val) {
                VECTOR::AudioManager::Get().SetMusicVolume(val);
            }));

        } else if (m_State == GameState::Settings) {
            VECTOR::Entity res1 = m_UIRegistry.CreateEntity();
            m_UIRegistry.AddComponent(res1, VECTOR::UIRectComponent(startX, startY, btnWidth, btnHeight, glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_UIRegistry.AddComponent(res1, VECTOR::UITextComponent("1280x720", 24, glm::vec4(1.0f)));
            auto& r1Text = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(res1);
            r1Text.offsetX = (btnWidth / 2) - (std::string("1280x720").length() * 6);
            r1Text.offsetY = (btnHeight / 2) - 12;
            m_UIRegistry.AddComponent(res1, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                []() { VECTOR::Application::Get().SetResolution(1280, 720); }
            ));

            VECTOR::Entity res2 = m_UIRegistry.CreateEntity();
            m_UIRegistry.AddComponent(res2, VECTOR::UIRectComponent(startX, startY + 60, btnWidth, btnHeight, glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_UIRegistry.AddComponent(res2, VECTOR::UITextComponent("1920x1080", 24, glm::vec4(1.0f)));
            auto& r2Text = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(res2);
            r2Text.offsetX = (btnWidth / 2) - (std::string("1920x1080").length() * 6);
            r2Text.offsetY = (btnHeight / 2) - 12;
            m_UIRegistry.AddComponent(res2, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                []() { VECTOR::Application::Get().SetResolution(1920, 1080); }
            ));

            VECTOR::Entity fsWin = m_UIRegistry.CreateEntity();
            m_UIRegistry.AddComponent(fsWin, VECTOR::UIRectComponent(startX - 110, startY + 120, btnWidth, btnHeight, glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_UIRegistry.AddComponent(fsWin, VECTOR::UITextComponent("Windowed", 24, glm::vec4(1.0f)));
            auto& fswText = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(fsWin);
            fswText.offsetX = (btnWidth / 2) - (std::string("Windowed").length() * 6);
            fswText.offsetY = (btnHeight / 2) - 12;
            m_UIRegistry.AddComponent(fsWin, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                []() { VECTOR::Application::Get().SetFullscreen(false, false); }
            ));

            VECTOR::Entity fsB = m_UIRegistry.CreateEntity();
            m_UIRegistry.AddComponent(fsB, VECTOR::UIRectComponent(startX + 110, startY + 120, btnWidth, btnHeight, glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_UIRegistry.AddComponent(fsB, VECTOR::UITextComponent("Borderless", 24, glm::vec4(1.0f)));
            auto& fsbText = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(fsB);
            fsbText.offsetX = (btnWidth / 2) - (std::string("Borderless").length() * 6);
            fsbText.offsetY = (btnHeight / 2) - 12;
            m_UIRegistry.AddComponent(fsB, VECTOR::UIButtonComponent(
                glm::vec4(50/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                []() { VECTOR::Application::Get().SetFullscreen(true, true); }
            ));

            VECTOR::Entity backBtn = m_UIRegistry.CreateEntity();
            m_UIRegistry.AddComponent(backBtn, VECTOR::UIRectComponent(startX, startY + 180, btnWidth, btnHeight, glm::vec4(100/255.0f, 50/255.0f, 50/255.0f, 1.0f)));
            m_UIRegistry.AddComponent(backBtn, VECTOR::UITextComponent("Back", 24, glm::vec4(1.0f)));
            auto& bText = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(backBtn);
            bText.offsetX = (btnWidth / 2) - (std::string("Back").length() * 6);
            bText.offsetY = (btnHeight / 2) - 12;
            m_UIRegistry.AddComponent(backBtn, VECTOR::UIButtonComponent(
                glm::vec4(100/255.0f, 50/255.0f, 50/255.0f, 1.0f), glm::vec4(200/255.0f, 100/255.0f, 100/255.0f, 1.0f),
                [this]() {
                    m_State = GameState::Paused;
                    m_NeedsUIRefresh = true;
                }
            ));
        }
    }

    GameplayScene::~GameplayScene() {
        m_InputManager->SetRelativeMouseMode(false);
        m_Registry.Clear();
    }

    // CreateColorMaterial removed

    void GameplayScene::GenerateArena() {
        // Editor Camera
        m_EditorCamera = m_Registry.CreateEntity();
        m_Registry.AddComponent(m_EditorCamera, VECTOR::TagComponent{"Editor Camera"});
        m_Registry.AddComponent(m_EditorCamera, VECTOR::TransformComponent{glm::vec3(0.0f, 10.0f, 10.0f)});
        auto eCam = VECTOR::CameraComponent{};
        eCam.pitch = -45.0f;
        eCam.isActive = true;
        m_Registry.AddComponent(m_EditorCamera, eCam);

        // Player Camera
        m_Player = m_Registry.CreateEntity();
        m_Registry.AddComponent(m_Player, VECTOR::TagComponent{"Player Camera"});
        m_Registry.AddComponent(m_Player, VECTOR::TransformComponent{glm::vec3(0.0f, 2.0f, 5.0f)});
        auto pCam = VECTOR::CameraComponent{};
        pCam.isActive = false;
        m_Registry.AddComponent(m_Player, pCam);

        btCollisionShape* playerShape = new btCapsuleShape(0.5f, 1.0f);
        btTransform playerStartTransform;
        playerStartTransform.setIdentity();
        playerStartTransform.setOrigin(btVector3(0.0f, 2.0f, 5.0f));

        btScalar playerMass(50.0f);
        btVector3 playerLocalInertia(0, 0, 0);
        playerShape->calculateLocalInertia(playerMass, playerLocalInertia);

        btDefaultMotionState* playerMotionState = new btDefaultMotionState(playerStartTransform);
        btRigidBody::btRigidBodyConstructionInfo playerRbInfo(playerMass, playerMotionState, playerShape, playerLocalInertia);
        btRigidBody* playerBody = new btRigidBody(playerRbInfo);
        playerBody->setFriction(0.8f); // Added friction
        // lock rotations
        playerBody->setAngularFactor(btVector3(0,0,0));
        
        auto deleter = [world = m_PhysicsSystem->GetWorld()](btRigidBody* rb) {
            if (world && rb) world->removeRigidBody(rb);
            if (rb) {
                if (rb->getMotionState()) delete rb->getMotionState();
                if (rb->getCollisionShape()) delete rb->getCollisionShape();
                delete rb;
            }
        };
        std::shared_ptr<btRigidBody> bodyPtr(playerBody, deleter);

        m_PhysicsSystem->GetWorld()->addRigidBody(playerBody);
        m_Registry.AddComponent(m_Player, VECTOR::RigidBodyComponent{bodyPtr});

        // Floor
        CreateCube(glm::vec3(0, -1, 0), glm::vec3(50, 1, 50), 0.0f, "assets/materials/floor.vmat");

        // Targets with different weights
        CreateCube(glm::vec3(0, 1, -10), glm::vec3(2, 2, 2), 10.0f, "assets/materials/medium_target.vmat"); // Medium
        CreateCube(glm::vec3(5, 1, -10), glm::vec3(1, 1, 1), 1.0f, "assets/materials/light_target.vmat");  // Light
        CreateCube(glm::vec3(-5, 1, -10), glm::vec3(3, 3, 3), 100.0f, "assets/materials/heavy_target.vmat"); // Heavy
    }

    void GameplayScene::CreateCube(const glm::vec3& position, const glm::vec3& scale, float mass, const std::string& materialPath, bool isEnemy) {
        VECTOR::Entity entity = m_Registry.CreateEntity();
        
        std::string name = "Cube";
        if (mass == 100.0f) name = "Heavy Target";
        else if (mass == 10.0f) name = "Medium Target";
        else if (mass == 1.0f) name = "Light Target";
        else if (mass == 0.0f) name = "Floor";
        m_Registry.AddComponent(entity, VECTOR::TagComponent{name});
        
        VECTOR::TransformComponent t;
        t.position = position;
        t.scale = scale;
        m_Registry.AddComponent(entity, t);

        auto mat = VECTOR::ResourceManager::Get().LoadMaterial(materialPath, materialPath);
        
        // Add point lights for visual flair on targets
        if (mass == 100.0f) {
            m_Registry.AddComponent(entity, VECTOR::PointLightComponent(glm::vec3(1.0f, 0.0f, 0.0f), 2.0f, 15.0f));
        } else if (mass == 10.0f) {
            m_Registry.AddComponent(entity, VECTOR::PointLightComponent(glm::vec3(0.0f, 0.0f, 1.0f), 1.0f, 10.0f));
        }

        m_Registry.AddComponent(entity, VECTOR::RenderComponent(mat));

        VECTOR::MeshComponent m;
        m.mesh = m_CubeMesh;
        m_Registry.AddComponent(entity, m);

        btCollisionShape* colShape = new btBoxShape(btVector3(scale.x/2.0f, scale.y/2.0f, scale.z/2.0f));
        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(position.x, position.y, position.z));

        btVector3 localInertia(0, 0, 0);
        if (mass > 0.0f) colShape->calculateLocalInertia(mass, localInertia);

        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
        btRigidBody* body = new btRigidBody(rbInfo);
        body->setFriction(0.8f); // Added friction
        
        auto deleter = [world = m_PhysicsSystem->GetWorld()](btRigidBody* rb) {
            if (world && rb) world->removeRigidBody(rb);
            if (rb) {
                if (rb->getMotionState()) delete rb->getMotionState();
                if (rb->getCollisionShape()) delete rb->getCollisionShape();
                delete rb;
            }
        };
        std::shared_ptr<btRigidBody> bodyPtr(body, deleter);
        m_PhysicsSystem->GetWorld()->addRigidBody(body);
        m_Registry.AddComponent(entity, VECTOR::RigidBodyComponent{bodyPtr});

        if (isEnemy) {
            m_Registry.AddComponent(entity, AIComponent{AIDifficulty::Medium, AIState::Tracking, 0.0f, 0.2f, 0.0f});
        }
    }

    // Mesh generated via Mesh::CreateCube

    void GameplayScene::OnEnter() {
    }

    void GameplayScene::Update(float deltaTime) {
        bool isEscapePressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_ESCAPE);
        bool isF3Pressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_F3);

        if (isF3Pressed && !m_WasF3Pressed) {
            m_DebugMode = !m_DebugMode;
        }
        m_WasF3Pressed = isF3Pressed;

        bool isRightClickHeld = m_InputManager->IsMouseButtonPressed(SDL_BUTTON_RIGHT);
        bool targetRelative = (m_State == GameState::Playing) || (m_State == GameState::Editor && isRightClickHeld);
        static bool lastRelative = true;
        if (targetRelative != lastRelative) {
            m_InputManager->SetRelativeMouseMode(targetRelative);
            lastRelative = targetRelative;
        }

        if (isEscapePressed && !m_WasEscapePressed) {
            if (m_State == GameState::Playing) {
                m_State = GameState::Paused;
                CreateUI();
            } else if (m_State == GameState::Paused) {
                m_State = GameState::Playing;
            } else if (m_State == GameState::Settings) {
                m_State = GameState::Paused;
                CreateUI();
            }
        }
        m_WasEscapePressed = isEscapePressed;

        if (m_State == GameState::Editor) {
            m_CameraSystem->m_RequireRightClick = true;
            m_CameraSystem->Update(m_Registry, deltaTime);
        } else if (m_State == GameState::Playing) {
            m_CameraSystem->m_RequireRightClick = false;
            for (auto& system : m_Systems) {
                system->Update(m_Registry, deltaTime);
            }
        } else {
            m_UISystem->Update(m_UIRegistry, deltaTime);
            if (m_NeedsUIRefresh) {
                CreateUI();
                m_NeedsUIRefresh = false;
            }
        }
    }

    void GameplayScene::Render(VECTOR::Renderer* renderer) {
        VECTOR::Entity activeCameraEntity = m_Player;
        if (m_State == GameState::Editor) {
            activeCameraEntity = m_EditorCamera;
        }
        
        auto& camT = m_Registry.GetComponent<VECTOR::TransformComponent>(activeCameraEntity);
        auto& camC = m_Registry.GetComponent<VECTOR::CameraComponent>(activeCameraEntity);

        glm::mat4 view = glm::lookAt(camT.position, camT.position + camC.front, camC.up);
        glm::mat4 projection = glm::perspective(glm::radians(camC.fov), (float)m_Width / (float)m_Height, 0.1f, 100.0f);
        
        renderer->SetViewProjection(camT.position, view, projection);

        // Submit Directional Light
        glm::vec3 sunDir = glm::normalize(glm::vec3(-0.2f, 1.0f, 0.3f));
        // Pass -sunDir because the shader expects the direction the light is travelling
        renderer->SetDirectionalLight(-sunDir, glm::vec3(1.0f, 0.95f, 0.9f), 1.5f);

        // Submit Point Lights
        m_Registry.View<VECTOR::TransformComponent, VECTOR::PointLightComponent>([&](VECTOR::Entity entity) {
            auto& t = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& l = m_Registry.GetComponent<VECTOR::PointLightComponent>(entity);
            renderer->SubmitPointLight(t.position, l.radius, l.color, l.intensity);
        });

        // Submit all renderable entities to the render queue
        m_Registry.View<VECTOR::TransformComponent, VECTOR::MeshComponent, VECTOR::RenderComponent>([&](VECTOR::Entity entity) {
            auto& t = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& m = m_Registry.GetComponent<VECTOR::MeshComponent>(entity);
            auto& r = m_Registry.GetComponent<VECTOR::RenderComponent>(entity);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.position);
            model = model * glm::mat4_cast(t.rotation);
            model = glm::scale(model, t.scale);

            renderer->SubmitMesh(m.mesh.get(), r.material.get(), model);
        });

        // Submit the sun (unlit)
        glm::mat4 sunModel = glm::mat4(1.0f);
        sunModel = glm::translate(sunModel, sunDir * 80.0f);
        sunModel = glm::scale(sunModel, glm::vec3(10.0f, 10.0f, 10.0f));
        
        VECTOR::Material sunMat;
        sunMat.isUnlit = true;
        sunMat.albedoColor = glm::vec4(3.0f, 3.0f, 2.4f, 1.0f);
        sunMat.shader = VECTOR::ResourceManager::Get().GetShader("Default3D");
        // It will be drawn in the main pass
        renderer->SubmitMesh(m_CubeMesh.get(), &sunMat, sunModel);

        // PASS 1: SHADOW MAP
        renderer->BeginShadowPass();
        renderer->FlushShadowPass();

        // PASS 2: MAIN POST-PROCESS FBO
        renderer->BeginMainPass();
        renderer->Clear(135, 206, 235); // Sky blue
        
        renderer->SetWireframeMode(m_DebugMode);

        renderer->FlushMainPass();

        // PASS 3: POST PROCESS SCREEN QUAD
        renderer->SetWireframeMode(false);
        renderer->EndPostProcessPass();

        renderer->BeginUI();
        // Crosshair
        renderer->DrawUIRect(m_Width / 2 - 2, m_Height / 2 - 2, 4, 4, glm::vec4(1.0f, 1.0f, 1.0f, 200/255.0f));

        // Raw text debug info has been migrated to the Dear ImGui Debugger window.

        if (m_State == GameState::Paused || m_State == GameState::Settings) {
            renderer->DrawUIRect(0, 0, m_Width, m_Height, glm::vec4(0.0f, 0.0f, 0.0f, 150/255.0f)); // Dark overlay
            if (m_State == GameState::Paused) {
                renderer->DrawUIText("PAUSED", m_Width / 2 - 80, m_Height / 2 - 120, glm::vec4(1.0f), 48);
                renderer->DrawUIText("Volume", m_Width - 250, 30, glm::vec4(200/255.0f, 200/255.0f, 200/255.0f, 1.0f), 18);
            } else if (m_State == GameState::Settings) {
                renderer->DrawUIText("Settings", m_Width / 2 - 80, m_Height / 2 - 120, glm::vec4(1.0f), 48);
            }
            
            // Render ECS UI
            m_UIRegistry.View<VECTOR::UIRectComponent>([&](VECTOR::Entity entity) {
                auto& rect = m_UIRegistry.GetComponent<VECTOR::UIRectComponent>(entity);
                if (!rect.isVisible) return;
                
                if (m_UIRegistry.HasComponent<VECTOR::UISliderComponent>(entity)) {
                    auto& slider = m_UIRegistry.GetComponent<VECTOR::UISliderComponent>(entity);
                    renderer->DrawUIRect(rect.x, rect.y, rect.width, rect.height, glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f));
                    int fillWidth = (int)(rect.width * slider.value);
                    renderer->DrawUIRect(rect.x, rect.y, fillWidth, rect.height, glm::vec4(50/255.0f, 200/255.0f, 50/255.0f, 1.0f));
                    
                    int knobWidth = 10;
                    int knobHeight = rect.height + 10;
                    int knobX = rect.x + fillWidth - (knobWidth / 2);
                    int knobY = rect.y - 5;
                    renderer->DrawUIRect(knobX, knobY, knobWidth, knobHeight, glm::vec4(1.0f));
                } else {
                    renderer->DrawUIRect(rect.x, rect.y, rect.width, rect.height, rect.color);
                }
                
                if (m_UIRegistry.HasComponent<VECTOR::UITextComponent>(entity)) {
                    auto& text = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(entity);
                    renderer->DrawUIText(text.text, rect.x + text.offsetX, rect.y + text.offsetY, text.color, text.fontSize);
                }
            });
        }
        renderer->EndUI();

        // Render Dear ImGui Debug HUD
        if (m_State == GameState::Editor || m_DebugMode) {
            EditorAction action = m_EditorUI->Render(m_State == GameState::Playing);
            
            if (action == EditorAction::Play) {
                m_State = GameState::Playing;
                m_InitialTransforms.clear();
                m_Registry.View<VECTOR::TransformComponent>([&](VECTOR::Entity entity) {
                    m_InitialTransforms[entity] = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
                });
                
                if (m_Registry.HasComponent<VECTOR::CameraComponent>(m_EditorCamera)) {
                    m_Registry.GetComponent<VECTOR::CameraComponent>(m_EditorCamera).isActive = false;
                }
                if (m_Registry.HasComponent<VECTOR::CameraComponent>(m_Player)) {
                    m_Registry.GetComponent<VECTOR::CameraComponent>(m_Player).isActive = true;
                }
                VECTOR::AudioManager::Get().PlayMusic("assets/bgm.wav");
            } else if (action == EditorAction::Stop) {
                m_State = GameState::Editor;
                for (auto& pair : m_InitialTransforms) {
                    VECTOR::Entity entity = pair.first;
                    auto& transform = pair.second;
                    
                    if (m_Registry.HasComponent<VECTOR::TransformComponent>(entity)) {
                        m_Registry.GetComponent<VECTOR::TransformComponent>(entity) = transform;
                    }
                    if (m_Registry.HasComponent<VECTOR::RigidBodyComponent>(entity)) {
                        auto& rb = m_Registry.GetComponent<VECTOR::RigidBodyComponent>(entity);
                        if (rb.body) {
                            btTransform startTransform;
                            startTransform.setIdentity();
                            startTransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
                            
                            // glm::quat to btQuaternion
                            btQuaternion btq(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
                            startTransform.setRotation(btq);
                            
                            rb.body->setWorldTransform(startTransform);
                            rb.body->setLinearVelocity(btVector3(0,0,0));
                            rb.body->setAngularVelocity(btVector3(0,0,0));
                            rb.body->clearForces();
                        }
                    }
                }
                
                if (m_Registry.HasComponent<VECTOR::CameraComponent>(m_EditorCamera)) {
                    m_Registry.GetComponent<VECTOR::CameraComponent>(m_EditorCamera).isActive = true;
                }
                if (m_Registry.HasComponent<VECTOR::CameraComponent>(m_Player)) {
                    m_Registry.GetComponent<VECTOR::CameraComponent>(m_Player).isActive = false;
                }
                VECTOR::AudioManager::Get().StopMusic();
            }
            
            ImGui::Begin("VECTOR Engine Debugger", &m_DebugMode);
            
            if (ImGui::CollapsingHeader("System Info", ImGuiTreeNodeFlags_DefaultOpen)) {
                float fps = VECTOR::Application::Get().GetFPS();
                ImGui::Text("FPS: %.1f (%.3f ms/frame)", fps, fps > 0.0f ? 1000.0f / fps : 0.0f);
                ImGui::Text("Draw Calls: %u", renderer->GetDrawCallCount());
                ImGui::Text("GPU: %s", renderer->GetRendererInfo().c_str());
                ImGui::Text("CPU Cores: %d", SDL_GetNumLogicalCPUCores());
                ImGui::Text("System RAM: %d MB", SDL_GetSystemRAM());
            }

            if (m_CameraSystem && ImGui::CollapsingHeader("Player & Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Position: X:%.2f, Y:%.2f, Z:%.2f", camT.position.x, camT.position.y, camT.position.z);
                ImGui::Text("Front: X:%.2f, Y:%.2f, Z:%.2f", camC.front.x, camC.front.y, camC.front.z);
                ImGui::SliderFloat("FOV", &camC.fov, 10.0f, 120.0f, "%.1f");
                ImGui::SliderFloat("Camera Speed", &m_CameraSystem->m_MovementSpeed, 1.0f, 50.0f, "%.1f");
                ImGui::SliderFloat("Mouse Sensitivity", &m_CameraSystem->m_MouseSensitivity, 0.01f, 1.0f, "%.2f");
            }

            if (m_PhysicsSystem && ImGui::CollapsingHeader("Physics System", ImGuiTreeNodeFlags_DefaultOpen)) {
                btVector3 grav = m_PhysicsSystem->GetWorld()->getGravity();
                float gravityY = grav.getY();
                if (ImGui::SliderFloat("Gravity Y", &gravityY, -30.0f, 30.0f, "%.1f")) {
                    m_PhysicsSystem->GetWorld()->setGravity(btVector3(0, gravityY, 0));
                }
            }

            ImGui::End();
        }
    }

}
