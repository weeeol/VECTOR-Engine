#include "Game/Core/EntityFactory.hpp"
#include "Engine/ECS/Components.hpp"
#include "Game/Core/GameComponents.hpp"
#include "Engine/Physics/PhysicsUtils.hpp"
#include <SDL3/SDL_scancode.h>

namespace Game {

    void EntityFactory::CreateWalls(VECTOR::Box2DPhysicsSystem* physics, int width, int height) {
        VECTOR::PhysicsUtils::CreateBox(physics->GetWorld(), width/2.0f, -10.0f, width, 20.0f, b2_staticBody, 0.0f, 0.0f, 1.0f, false, (void*)0); // Top
        VECTOR::PhysicsUtils::CreateBox(physics->GetWorld(), width/2.0f, height + 10.0f, width, 20.0f, b2_staticBody, 0.0f, 0.0f, 1.0f, false, (void*)0); // Bottom
    }

    void EntityFactory::CreateGoals(VECTOR::Box2DPhysicsSystem* physics, int width, int height) {
        VECTOR::PhysicsUtils::CreateBox(physics->GetWorld(), -10.0f, height/2.0f, 20.0f, height, b2_staticBody, 0.0f, 0.0f, 0.0f, true, (void*)2); // Left Goal
        VECTOR::PhysicsUtils::CreateBox(physics->GetWorld(), width + 10.0f, height/2.0f, 20.0f, height, b2_staticBody, 0.0f, 0.0f, 0.0f, true, (void*)3); // Right Goal
    }

    VECTOR::Entity EntityFactory::CreatePlayer1(VECTOR::Registry& registry, VECTOR::Box2DPhysicsSystem* physics, int width, int height) {
        VECTOR::Entity player1 = registry.CreateEntity();
        registry.AddComponent(player1, VECTOR::TransformComponent{{30.0f, height / 2.0f - 50.0f}});
        registry.AddComponent(player1, VECTOR::RenderComponent{20.0f, 100.0f, 60, 200, 255, 255});
        registry.AddComponent(player1, PlayerInputComponent{SDL_SCANCODE_W, SDL_SCANCODE_S});
        
        b2BodyId p1Body = VECTOR::PhysicsUtils::CreateBox(physics->GetWorld(), 30.0f + 10.0f, height / 2.0f, 20.0f, 100.0f, b2_kinematicBody, 1.0f, 0.0f, 1.0f, false, (void*)4);
        registry.AddComponent(player1, VECTOR::RigidBodyComponent{p1Body});
        return player1;
    }

    VECTOR::Entity EntityFactory::CreatePlayer2(VECTOR::Registry& registry, VECTOR::Box2DPhysicsSystem* physics, int width, int height, AIDifficulty difficulty) {
        VECTOR::Entity player2 = registry.CreateEntity();
        registry.AddComponent(player2, VECTOR::TransformComponent{{width - 50.0f, height / 2.0f - 50.0f}});
        registry.AddComponent(player2, VECTOR::RenderComponent{20.0f, 100.0f, 255, 140, 50, 255});
        registry.AddComponent(player2, AIComponent{difficulty, AIState::Idle, 0.0f, 0.2f, height / 2.0f});
        
        b2BodyId p2Body = VECTOR::PhysicsUtils::CreateBox(physics->GetWorld(), width - 50.0f + 10.0f, height / 2.0f, 20.0f, 100.0f, b2_kinematicBody, 1.0f, 0.0f, 1.0f, false, (void*)4);
        registry.AddComponent(player2, VECTOR::RigidBodyComponent{p2Body});
        return player2;
    }

    VECTOR::Entity EntityFactory::CreateBall(VECTOR::Registry& registry, VECTOR::Box2DPhysicsSystem* physics, int width, int height) {
        VECTOR::Entity ball = registry.CreateEntity();
        registry.AddComponent(ball, VECTOR::TransformComponent{{width / 2.0f, height / 2.0f}});
        registry.AddComponent(ball, VECTOR::RenderComponent{15.0f, 15.0f, 255, 255, 255, 255});
        registry.AddComponent(ball, BallComponent{true});
        
        b2BodyId ballBody = VECTOR::PhysicsUtils::CreateBox(physics->GetWorld(), width / 2.0f + 7.5f, height / 2.0f + 7.5f, 15.0f, 15.0f, b2_dynamicBody, 1.0f, 0.0f, 1.05f, false, (void*)1);
        registry.AddComponent(ball, VECTOR::RigidBodyComponent{ballBody});
        return ball;
    }

}
