#pragma once

#include <glm/glm.hpp>
#include <array>

namespace VECTOR {

    struct AABB {
        glm::vec3 center{0.f, 0.f, 0.f};
        glm::vec3 extents{0.f, 0.f, 0.f};

        AABB() = default;
        AABB(const glm::vec3& inCenter, const glm::vec3& inExtents)
            : center(inCenter), extents(inExtents) {}
    };

    struct Plane {
        glm::vec3 normal{0.f, 1.f, 0.f};
        float distance{0.f}; // Used as dot(normal, point) + distance

        Plane() = default;
        Plane(const glm::vec3& p1, const glm::vec3& norm)
            : normal(glm::normalize(norm)), distance(-glm::dot(normal, p1)) {}

        float getSignedDistanceToPlane(const glm::vec3& point) const {
            return glm::dot(normal, point) + distance;
        }
    };

    struct Frustum {
        Plane topFace;
        Plane bottomFace;
        Plane rightFace;
        Plane leftFace;
        Plane farFace;
        Plane nearFace;
    };

    /**
     * @brief Creates a frustum directly from a view-projection matrix
     */
    Frustum CreateFrustumFromMatrix(const glm::mat4& vp);

    /**
     * @brief Tests if an AABB is within a frustum
     */
    bool IsOnFrustum(const Frustum& frustum, const AABB& aabb);

} // namespace VECTOR
