#pragma once

#include "Engine/ECS/ECS.hpp"
#include "Engine/Physics/Box2DPhysicsSystem.hpp"
#include "Game/Events/GameEvents.hpp"

namespace Game {
    class EntityFactory {
    public:
        static void CreateWalls(VECTOR::Box2DPhysicsSystem* physics, int width, int height);
        static void CreateGoals(VECTOR::Box2DPhysicsSystem* physics, int width, int height);
        static VECTOR::Entity CreatePlayer1(VECTOR::Registry& registry, VECTOR::Box2DPhysicsSystem* physics, int width, int height);
        static VECTOR::Entity CreatePlayer2(VECTOR::Registry& registry, VECTOR::Box2DPhysicsSystem* physics, int width, int height, AIDifficulty difficulty);
        static VECTOR::Entity CreateBall(VECTOR::Registry& registry, VECTOR::Box2DPhysicsSystem* physics, int width, int height);
    };
}
