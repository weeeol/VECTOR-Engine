#include "Engine/Graphics/Frustum.hpp"

namespace VECTOR {

    Frustum CreateFrustumFromMatrix(const glm::mat4& vp) {
        Frustum frustum;

        // Left
        frustum.leftFace.normal.x = vp[0][3] + vp[0][0];
        frustum.leftFace.normal.y = vp[1][3] + vp[1][0];
        frustum.leftFace.normal.z = vp[2][3] + vp[2][0];
        frustum.leftFace.distance = vp[3][3] + vp[3][0];

        // Right
        frustum.rightFace.normal.x = vp[0][3] - vp[0][0];
        frustum.rightFace.normal.y = vp[1][3] - vp[1][0];
        frustum.rightFace.normal.z = vp[2][3] - vp[2][0];
        frustum.rightFace.distance = vp[3][3] - vp[3][0];
        
        // Bottom
        frustum.bottomFace.normal.x = vp[0][3] + vp[0][1];
        frustum.bottomFace.normal.y = vp[1][3] + vp[1][1];
        frustum.bottomFace.normal.z = vp[2][3] + vp[2][1];
        frustum.bottomFace.distance = vp[3][3] + vp[3][1];

        // Top
        frustum.topFace.normal.x = vp[0][3] - vp[0][1];
        frustum.topFace.normal.y = vp[1][3] - vp[1][1];
        frustum.topFace.normal.z = vp[2][3] - vp[2][1];
        frustum.topFace.distance = vp[3][3] - vp[3][1];

        // Near
        frustum.nearFace.normal.x = vp[0][3] + vp[0][2];
        frustum.nearFace.normal.y = vp[1][3] + vp[1][2];
        frustum.nearFace.normal.z = vp[2][3] + vp[2][2];
        frustum.nearFace.distance = vp[3][3] + vp[3][2];

        // Far
        frustum.farFace.normal.x = vp[0][3] - vp[0][2];
        frustum.farFace.normal.y = vp[1][3] - vp[1][2];
        frustum.farFace.normal.z = vp[2][3] - vp[2][2];
        frustum.farFace.distance = vp[3][3] - vp[3][2];

        auto normalizePlane = [](Plane& p) {
            float mag = glm::length(p.normal);
            p.normal /= mag;
            p.distance /= mag;
        };

        normalizePlane(frustum.leftFace);
        normalizePlane(frustum.rightFace);
        normalizePlane(frustum.bottomFace);
        normalizePlane(frustum.topFace);
        normalizePlane(frustum.nearFace);
        normalizePlane(frustum.farFace);

        return frustum;
    }

    bool IsOnOrForwardPlane(const Plane& plane, const AABB& aabb) {
        const float r = aabb.extents.x * std::abs(plane.normal.x) +
                        aabb.extents.y * std::abs(plane.normal.y) +
                        aabb.extents.z * std::abs(plane.normal.z);
        return -r <= plane.getSignedDistanceToPlane(aabb.center);
    }

    bool IsOnFrustum(const Frustum& frustum, const AABB& aabb) {
        return (IsOnOrForwardPlane(frustum.leftFace, aabb) &&
                IsOnOrForwardPlane(frustum.rightFace, aabb) &&
                IsOnOrForwardPlane(frustum.topFace, aabb) &&
                IsOnOrForwardPlane(frustum.bottomFace, aabb) &&
                IsOnOrForwardPlane(frustum.nearFace, aabb) &&
                IsOnOrForwardPlane(frustum.farFace, aabb));
    }

} // namespace VECTOR
