#pragma once

#include "Engine/ECS/System.hpp"
#include "Engine/ECS/ECS.hpp"
#include <box2d/box2d.h>

namespace VECTOR {

    constexpr float PIXELS_PER_METER = 50.0f;

    class Box2DPhysicsSystem : public System {
    public:
        Box2DPhysicsSystem();
        ~Box2DPhysicsSystem() override;

        void Update(Registry& registry, float deltaTime) override;

        b2WorldId GetWorld() { return m_WorldId; }

    private:
        b2WorldId m_WorldId;
    };

} // namespace VECTOR
