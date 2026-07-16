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
#include "Engine/ECS/UIComponents.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Shader.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include "Engine/Audio/AudioManager.hpp"
#include <SDL.h>

namespace Game {

    GameplayScene::GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty)
        : m_Width(width), m_Height(height), m_InputManager(inputManager), m_IsPaused(false), m_WasPausePressed(false)
    {
        m_Registry.RegisterComponent<VECTOR::TransformComponent>();
        m_Registry.RegisterComponent<VECTOR::RigidBodyComponent>();
        m_Registry.RegisterComponent<VECTOR::RenderComponent>();
        m_Registry.RegisterComponent<VECTOR::MeshComponent>();
        m_Registry.RegisterComponent<VECTOR::CameraComponent>();
        m_Registry.RegisterComponent<AIComponent>(); // Reusing AIComponent for enemies

        auto physSys = std::make_unique<VECTOR::BulletPhysicsSystem>();
        m_PhysicsSystem = physSys.get();
        m_Systems.push_back(std::move(physSys));

        m_Systems.push_back(std::make_unique<CameraSystem>(m_InputManager));
        m_Systems.push_back(std::make_unique<ShootingSystem>(m_InputManager, m_PhysicsSystem));

        m_CubeMesh = VECTOR::Mesh::CreateCube();

        // UI Registry Setup
        m_UIRegistry.RegisterComponent<VECTOR::UIRectComponent>();
        m_UIRegistry.RegisterComponent<VECTOR::UITextComponent>();
        m_UIRegistry.RegisterComponent<VECTOR::UIButtonComponent>();
        m_UIRegistry.RegisterComponent<VECTOR::UISliderComponent>();

        m_UISystem = std::make_unique<VECTOR::UISystem>(m_InputManager);

        // Pause Menu UI
        VECTOR::Entity mainMenuBtn = m_UIRegistry.CreateEntity();
        m_UIRegistry.AddComponent(mainMenuBtn, VECTOR::UIRectComponent(width / 2 - 100, height / 2 + 50, 200, 50, glm::vec4(50/255.0f, 100/255.0f, 50/255.0f, 1.0f)));
        m_UIRegistry.AddComponent(mainMenuBtn, VECTOR::UITextComponent("Main Menu", 24, glm::vec4(1.0f)));
        
        auto& textC = m_UIRegistry.GetComponent<VECTOR::UITextComponent>(mainMenuBtn);
        textC.offsetX = 100 - (std::string("Main Menu").length() * 6);
        textC.offsetY = 25 - 12;

        m_UIRegistry.AddComponent(mainMenuBtn, VECTOR::UIButtonComponent(
            glm::vec4(50/255.0f, 100/255.0f, 50/255.0f, 1.0f),
            glm::vec4(100/255.0f, 200/255.0f, 100/255.0f, 1.0f),
            [this]() {
                auto menuScene = std::make_unique<MainMenuScene>(m_Width, m_Height, m_InputManager);
                VECTOR::SceneManager::Get().ChangeScene(std::move(menuScene));
            }
        ));

        VECTOR::Entity volSlider = m_UIRegistry.CreateEntity();
        m_UIRegistry.AddComponent(volSlider, VECTOR::UIRectComponent(width - 250, 50, 200, 20, glm::vec4(100/255.0f, 100/255.0f, 100/255.0f, 1.0f)));
        m_UIRegistry.AddComponent(volSlider, VECTOR::UISliderComponent(0.5f, [](float val) {
            VECTOR::AudioManager::Get().SetMusicVolume(val);
        }));

        GenerateArena();

        m_InputManager->SetRelativeMouseMode(true);
    }

    GameplayScene::~GameplayScene() {
        m_InputManager->SetRelativeMouseMode(false);
    }

    void GameplayScene::GenerateArena() {
        // Player Camera
        m_Player = m_Registry.CreateEntity();
        m_Registry.AddComponent(m_Player, VECTOR::TransformComponent{glm::vec3(0.0f, 2.0f, 5.0f)});
        m_Registry.AddComponent(m_Player, VECTOR::CameraComponent{});

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
        
        m_PhysicsSystem->GetWorld()->addRigidBody(playerBody);
        m_Registry.AddComponent(m_Player, VECTOR::RigidBodyComponent{playerBody});

        // Floor
        CreateCube(glm::vec3(0, -1, 0), glm::vec3(50, 1, 50), 0.0f, glm::vec3(0.3f, 0.3f, 0.3f));

        // Targets with different weights
        CreateCube(glm::vec3(0, 1, -10), glm::vec3(2, 2, 2), 10.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Medium
        CreateCube(glm::vec3(5, 1, -10), glm::vec3(1, 1, 1), 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));  // Light
        CreateCube(glm::vec3(-5, 1, -10), glm::vec3(3, 3, 3), 100.0f, glm::vec3(1.0f, 0.0f, 0.0f)); // Heavy
    }

    void GameplayScene::CreateCube(const glm::vec3& position, const glm::vec3& scale, float mass, const glm::vec3& color, bool isEnemy) {
        VECTOR::Entity entity = m_Registry.CreateEntity();
        VECTOR::TransformComponent t;
        t.position = position;
        t.scale = scale;
        m_Registry.AddComponent(entity, t);

        VECTOR::RenderComponent r;
        r.color = glm::vec4(color, 1.0f);
        m_Registry.AddComponent(entity, r);

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
        
        m_PhysicsSystem->GetWorld()->addRigidBody(body);
        m_Registry.AddComponent(entity, VECTOR::RigidBodyComponent{body});

        if (isEnemy) {
            m_Registry.AddComponent(entity, AIComponent{AIDifficulty::Medium, AIState::Tracking, 0.0f, 0.2f, 0.0f});
        }
    }

    // Mesh generated via Mesh::CreateCube

    void GameplayScene::OnEnter() {
    }

    void GameplayScene::Update(float deltaTime) {
        bool isPausePressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_ESCAPE);
        bool isF3Pressed = m_InputManager->IsKeyPressed(SDL_SCANCODE_F3);

        if (isF3Pressed && !m_WasF3Pressed) {
            m_DebugMode = !m_DebugMode;
        }
        m_WasF3Pressed = isF3Pressed;

        if (isPausePressed && !m_WasPausePressed) {
            m_IsPaused = !m_IsPaused;
            m_InputManager->SetRelativeMouseMode(!m_IsPaused);
        }
        m_WasPausePressed = isPausePressed;

        if (m_IsPaused) {
            m_UISystem->Update(m_UIRegistry, deltaTime);
        } else {
            for (auto& system : m_Systems) {
                system->Update(m_Registry, deltaTime);
            }
        }
    }

    void GameplayScene::Render(VECTOR::Renderer* renderer) {
        auto& camT = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Player);
        auto& camC = m_Registry.GetComponent<VECTOR::CameraComponent>(m_Player);

        glm::mat4 view = glm::lookAt(camT.position, camT.position + camC.front, camC.up);
        glm::mat4 projection = glm::perspective(glm::radians(camC.fov), (float)m_Width / (float)m_Height, 0.1f, 100.0f);
        
        renderer->SetViewProjection(camT.position, view, projection);

        // Helper lambda to render all scene meshes
        auto renderMeshes = [&](VECTOR::Shader* overrideShader) {
            m_Registry.View<VECTOR::TransformComponent, VECTOR::MeshComponent, VECTOR::RenderComponent>([&](VECTOR::Entity entity) {
                auto& t = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
                auto& m = m_Registry.GetComponent<VECTOR::MeshComponent>(entity);
                auto& r = m_Registry.GetComponent<VECTOR::RenderComponent>(entity);

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, t.position);
                model = model * glm::mat4_cast(t.rotation);
                model = glm::scale(model, t.scale);

                glm::vec4 color(r.color.r, r.color.g, r.color.b, r.color.a);
                
                renderer->DrawMesh(m.mesh.get(), overrideShader ? overrideShader : r.shader.get(), r.texture.get(), model, color);
            });
        };

        // PASS 1: SHADOW MAP
        renderer->BeginShadowPass();
        renderMeshes(renderer->GetDepthShader());

        // PASS 2: MAIN POST-PROCESS FBO
        renderer->BeginMainPass();
        renderer->Clear(135, 206, 235); // Sky blue
        
        if (m_DebugMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        renderMeshes(nullptr); // Uses default or component shaders

        // Explicitly draw the sun without shadows
        glm::vec3 sunDir = glm::normalize(glm::vec3(-0.2f, 1.0f, 0.3f));
        glm::mat4 sunModel = glm::mat4(1.0f);
        sunModel = glm::translate(sunModel, sunDir * 80.0f);
        sunModel = glm::scale(sunModel, glm::vec3(10.0f, 10.0f, 10.0f));
        renderer->SetUnlitMode(true);
        renderer->DrawMesh(m_CubeMesh.get(), nullptr, nullptr, sunModel, glm::vec4(1.0f, 1.0f, 0.8f, 1.0f));
        renderer->SetUnlitMode(false);

        // PASS 3: POST PROCESS SCREEN QUAD
        // Make sure polygon mode is filled before drawing full screen quad!
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        renderer->EndPostProcessPass();

        renderer->BeginUI();
        // Crosshair
        renderer->DrawUIRect(m_Width / 2 - 2, m_Height / 2 - 2, 4, 4, glm::vec4(1.0f, 1.0f, 1.0f, 200/255.0f));

        if (m_DebugMode) {
            float fps = VECTOR::Application::Get().GetFPS();
            
            std::string fpsStr = "FPS: " + std::to_string((int)fps);
            std::string posStr = "Pos: " + std::to_string(camT.position.x) + ", " + std::to_string(camT.position.y) + ", " + std::to_string(camT.position.z);
            
            int cpuCount = SDL_GetCPUCount();
            int ramMB = SDL_GetSystemRAM();
            std::string sysStr = "CPU Cores: " + std::to_string(cpuCount) + " | RAM: " + std::to_string(ramMB) + " MB";
            
            renderer->DrawUIText(fpsStr, 10, 10, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), 18);
            renderer->DrawUIText(posStr, 10, 30, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), 18);
            renderer->DrawUIText(sysStr, 10, 50, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), 18);
            
            const GLubyte* rendererStr = glGetString(GL_RENDERER);
            if (rendererStr) {
                std::string gpuStr = "GPU: " + std::string(reinterpret_cast<const char*>(rendererStr));
                renderer->DrawUIText(gpuStr, 10, 70, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), 18);
            }
        }

        if (m_IsPaused) {
            renderer->DrawUIRect(0, 0, m_Width, m_Height, glm::vec4(0.0f, 0.0f, 0.0f, 150/255.0f)); // Dark overlay
            renderer->DrawUIText("PAUSED", m_Width / 2 - 80, m_Height / 2 - 50, glm::vec4(1.0f), 48);
            renderer->DrawUIText("Volume", m_Width - 250, 30, glm::vec4(200/255.0f, 200/255.0f, 200/255.0f, 1.0f), 18);
            
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
    }

}

