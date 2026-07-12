#pragma once

namespace VECTOR {

    /**
     * @struct Vector2D
     * @brief A simple 2D vector for position and velocity.
     */
    struct Vector2D {
        float x;
        float y;

        Vector2D() : x(0.0f), y(0.0f) {}
        Vector2D(float x, float y) : x(x), y(y) {}

        Vector2D operator+(const Vector2D& rhs) const {
            return Vector2D(x + rhs.x, y + rhs.y);
        }

        Vector2D& operator+=(const Vector2D& rhs) {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        Vector2D operator*(float scalar) const {
            return Vector2D(x * scalar, y * scalar);
        }
    };

    /**
     * @struct AABB
     * @brief Axis-Aligned Bounding Box for basic collision detection.
     */
    struct AABB {
        float x, y;       // Top-left corner
        float w, h;       // Width and height

        // Check if this AABB is intersecting with another
        bool Intersects(const AABB& other) const {
            return (x < other.x + other.w &&
                    x + w > other.x &&
                    y < other.y + other.h &&
                    y + h > other.y);
        }
    };

} // namespace VECTOR
