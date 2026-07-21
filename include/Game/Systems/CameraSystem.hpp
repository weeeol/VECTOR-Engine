#pragma once

#include "Engine/ECS/ECS.hpp"
#include "Engine/ECS/System.hpp"
#include "Engine/Input/InputManager.hpp"
#include <glm/glm.hpp>

namespace Game {

    class CameraSystem : public VECTOR::System {
    public:
        CameraSystem(VECTOR::InputManager* inputManager);
        ~CameraSystem() override;

        void Update(VECTOR::Registry& registry, float deltaTime) override;

        float m_MouseSensitivity = 0.1f;
        float m_MovementSpeed = 10.0f;
        bool m_RequireRightClick = false;

    private:
        VECTOR::InputManager* m_InputManager;
    };

} // namespace Game
