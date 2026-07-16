#include "Game/Systems/CameraSystem.hpp"
#include "Engine/ECS/Components.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <btBulletDynamicsCommon.h>
#include <algorithm>

namespace Game {

    CameraSystem::CameraSystem(VECTOR::InputManager* inputManager) : m_InputManager(inputManager) {
    }

    CameraSystem::~CameraSystem() {
    }

    void CameraSystem::Update(VECTOR::Registry& registry, float deltaTime) {
        registry.View<VECTOR::TransformComponent, VECTOR::CameraComponent>([&](VECTOR::Entity entity) {
            auto& t = registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& cam = registry.GetComponent<VECTOR::CameraComponent>(entity);

            // Mouse Look
            float xoffset = m_InputManager->GetMouseDeltaX() * m_MouseSensitivity;
            float yoffset = -m_InputManager->GetMouseDeltaY() * m_MouseSensitivity; // reversed since y-coordinates go from bottom to top

            cam.yaw += xoffset;
            cam.pitch += yoffset;

            // Make sure that when pitch is out of bounds, screen doesn't get flipped
            if (cam.pitch > 89.0f) cam.pitch = 89.0f;
            if (cam.pitch < -89.0f) cam.pitch = -89.0f;

            // FOV adjustments
            if (m_InputManager->IsKeyPressed(VECTOR::KeyCode::Up))
                cam.fov -= 30.0f * deltaTime;
            if (m_InputManager->IsKeyPressed(VECTOR::KeyCode::Down))
                cam.fov += 30.0f * deltaTime;

            if (cam.fov < 10.0f) cam.fov = 10.0f;
            if (cam.fov > 120.0f) cam.fov = 120.0f;

            // Update Front, Right and Up Vectors using the updated Euler angles
            glm::vec3 front;
            front.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
            front.y = sin(glm::radians(cam.pitch));
            front.z = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
            cam.front = glm::normalize(front);
            
            // Also re-calculate the Right and Up vector
            cam.right = glm::normalize(glm::cross(cam.front, glm::vec3(0.0f, 1.0f, 0.0f)));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
            cam.up    = glm::normalize(glm::cross(cam.right, cam.front));

            // WASD Movement
            float velocityMag = m_MovementSpeed;
            glm::vec3 moveDir(0.0f);

            if (m_InputManager->IsKeyPressed(VECTOR::KeyCode::W))
                moveDir += cam.front;
            if (m_InputManager->IsKeyPressed(VECTOR::KeyCode::S))
                moveDir -= cam.front;
            if (m_InputManager->IsKeyPressed(VECTOR::KeyCode::A))
                moveDir -= cam.right;
            if (m_InputManager->IsKeyPressed(VECTOR::KeyCode::D))
                moveDir += cam.right;

            // Flatten movement direction on Y axis to prevent flying
            moveDir.y = 0.0f;
            if (glm::length(moveDir) > 0.0f) {
                moveDir = glm::normalize(moveDir);
            }

            if (registry.HasComponent<VECTOR::RigidBodyComponent>(entity)) {
                auto& rb = registry.GetComponent<VECTOR::RigidBodyComponent>(entity);
                if (rb.body) {
                    rb.body->activate(true);
                    btVector3 vel = rb.body->getLinearVelocity();
                    vel.setX(moveDir.x * velocityMag);
                    vel.setZ(moveDir.z * velocityMag);
                    rb.body->setLinearVelocity(vel);
                }
            } else {
                t.position += moveDir * velocityMag * deltaTime;
            }
        });
    }

} // namespace Game

