#pragma once

#include "Engine/ECS/ECS.hpp"

#include "Engine/ECS/System.hpp"

namespace Game {

    class ParticleSystem : public VECTOR::System {
    public:
        ParticleSystem() = default;
        ~ParticleSystem() = default;

        void Update(VECTOR::Registry& registry, float deltaTime) override;
    };

} // namespace Game
