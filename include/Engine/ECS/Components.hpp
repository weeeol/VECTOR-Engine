#pragma once
#include <box2d/box2d.h>
#include <stdint.h>

namespace VECTOR {
    struct Vector2D {
        float x, y;
    };

    struct TransformComponent {
        Vector2D position;
    };

    struct RigidBodyComponent {
        b2BodyId bodyId;
    };

    struct RenderComponent {
        float width, height;
        uint8_t r, g, b, a;
    };

    struct SpriteComponent {
        class Animator* animator;
    };
}
