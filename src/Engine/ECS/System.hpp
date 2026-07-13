#pragma once
#include "Engine/ECS/ECS.hpp"

namespace VECTOR {
    class System {
    public:
        virtual ~System() = default;
        
        // This is called every frame
        virtual void Update(Registry& registry, float deltaTime) = 0;
    };
}
