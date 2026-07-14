#pragma once
#include "Engine/ECS/System.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/ECS/UIComponents.hpp"

namespace VECTOR {
    class UISystem : public System {
    public:
        UISystem(InputManager* inputManager);
        
        virtual void Update(Registry& registry, float deltaTime) override;
        
    private:
        InputManager* m_Input;
    };
}
