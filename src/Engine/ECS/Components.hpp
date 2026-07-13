#pragma once
#include "Engine/Math/Math.hpp"
#include <cstdint>

namespace VECTOR {
    struct TransformComponent {
        Vector2D position;
    };

    struct VelocityComponent {
        Vector2D velocity;
    };

    struct RenderComponent {
        float width, height;
        uint8_t r, g, b, a;
    };

    struct ColliderComponent {
        float width, height;
    };
}
