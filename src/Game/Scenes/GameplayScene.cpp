#include "GameplayScene.hpp"
#include "MainMenuScene.hpp"
#include "Engine/Core/SceneManager.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Events/EventBus.hpp"
#include "Game/Events/GameEvents.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/Application.hpp"
#include "Engine/ECS/Components.hpp"
#include "Engine/UI/UIButton.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Shader.hpp"
#include "Engine/Graphics/Texture2D.hpp"

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

        // Pause Menu
        auto mainMenuButton = std::make_shared<VECTOR::UIButton>(width / 2 - 100, height / 2 + 50, 200, 50, "Main Menu", [this]() {
            auto menuScene = std::make_unique<MainMenuScene>(m_Width, m_Height, m_InputManager);
            VECTOR::SceneManager::Get().ChangeScene(std::move(menuScene));
        });
        m_PauseMenuUI.AddElement(mainMenuButton);

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
        
        // Prevent player from falling over
        playerBody->setAngularFactor(btVector3(0, 0, 0));
        
        m_PhysicsSystem->GetWorld()->addRigidBody(playerBody);
        m_Registry.AddComponent(m_Player, VECTOR::RigidBodyComponent{playerBody});

        // Floor
        CreateCube(glm::vec3(0, -1, 0), glm::vec3(50, 1, 50), 0.0f, glm::vec3(0.3f, 0.3f, 0.3f));

        // Targets (Static)
        CreateCube(glm::vec3(0, 1, -10), glm::vec3(2, 2, 2), 5.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        CreateCube(glm::vec3(5, 1, -10), glm::vec3(2, 2, 2), 5.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        CreateCube(glm::vec3(-5, 1, -10), glm::vec3(2, 2, 2), 5.0f, glm::vec3(1.0f, 0.0f, 0.0f));

        // Enemies (Moving targets)
        CreateCube(glm::vec3(10, 1, -15), glm::vec3(1.5f, 1.5f, 1.5f), 2.0f, glm::vec3(1.0f, 0.5f, 0.0f), true);
        CreateCube(glm::vec3(-10, 1, -15), glm::vec3(1.5f, 1.5f, 1.5f), 2.0f, glm::vec3(1.0f, 0.5f, 0.0f), true);
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

        if (isPausePressed && !m_WasPausePressed) {
            m_IsPaused = !m_IsPaused;
            m_InputManager->SetRelativeMouseMode(!m_IsPaused);
        }
        m_WasPausePressed = isPausePressed;

        if (m_IsPaused) {
            m_PauseMenuUI.Update(m_InputManager, deltaTime);
        } else {
            for (auto& system : m_Systems) {
                system->Update(m_Registry, deltaTime);
            }

            // Simple Enemy AI
            auto& camT = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Player);
            m_Registry.View<VECTOR::TransformComponent, VECTOR::RigidBodyComponent, AIComponent>([&](VECTOR::Entity entity) {
                auto& t = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
                auto& rb = m_Registry.GetComponent<VECTOR::RigidBodyComponent>(entity);
                
                glm::vec3 dir = camT.position - t.position;
                dir.y = 0; // Don't fly
                if (glm::length(dir) > 0.0f) {
                    dir = glm::normalize(dir);
                    btVector3 vel = rb.body->getLinearVelocity();
                    vel.setX(dir.x * 3.0f);
                    vel.setZ(dir.z * 3.0f);
                    rb.body->setLinearVelocity(vel);
                }
            });
        }
    }

    void GameplayScene::Render(VECTOR::Renderer* renderer) {
        renderer->Clear(135, 206, 235); // Sky blue

        auto& camT = m_Registry.GetComponent<VECTOR::TransformComponent>(m_Player);
        auto& camC = m_Registry.GetComponent<VECTOR::CameraComponent>(m_Player);

        glm::mat4 view = glm::lookAt(camT.position, camT.position + camC.front, camC.up);
        glm::mat4 projection = glm::perspective(glm::radians(camC.fov), (float)m_Width / (float)m_Height, 0.1f, 100.0f);
        
        renderer->SetViewProjection(view, projection);

        m_Registry.View<VECTOR::TransformComponent, VECTOR::MeshComponent, VECTOR::RenderComponent>([&](VECTOR::Entity entity) {
            auto& t = m_Registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& m = m_Registry.GetComponent<VECTOR::MeshComponent>(entity);
            auto& r = m_Registry.GetComponent<VECTOR::RenderComponent>(entity);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.position);
            model = model * glm::mat4_cast(t.rotation);
            model = glm::scale(model, t.scale);

            glm::vec4 color(r.color.r, r.color.g, r.color.b, r.color.a);
            
            renderer->DrawMesh(m.mesh.get(), r.shader.get(), r.texture.get(), model, color);
        });

        // Crosshair
        renderer->DrawRect(m_Width / 2 - 2, m_Height / 2 - 2, 4, 4, 255, 255, 255, 200);

        if (m_IsPaused) {
            renderer->DrawRect(0, 0, m_Width, m_Height, 0, 0, 0, 150); // Dark overlay
            renderer->DrawText("PAUSED", m_Width / 2 - 80, m_Height / 2 - 50, 255, 255, 255, 48);
            m_PauseMenuUI.Render(renderer);
        }
    }

}
