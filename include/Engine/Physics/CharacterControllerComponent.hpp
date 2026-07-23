#pragma once
#include <memory>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

namespace VECTOR {

    struct CharacterControllerComponent {
        std::shared_ptr<btPairCachingGhostObject> ghostObject;
        std::shared_ptr<btKinematicCharacterController> character;
        float walkSpeed = 10.0f;
        float jumpSpeed = 5.0f;
        float fallSpeed = 55.0f;
        float maxSlopeAngle = 45.0f; // degrees

        CharacterControllerComponent() = default;
        ~CharacterControllerComponent() = default;
    };

} // namespace VECTOR
