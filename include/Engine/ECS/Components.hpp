#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stdint.h>
#include <memory>

class btRigidBody;

namespace VECTOR {

    class Material;

    struct TransformComponent {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
        
        TransformComponent() : position(0.0f), rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), scale(1.0f) {}
        TransformComponent(const glm::vec3& p) : position(p), rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), scale(1.0f) {}
    };

    struct RigidBodyComponent {
        ::btRigidBody* body;
    };

    struct RenderComponent {
        std::shared_ptr<Material> material;

        RenderComponent() = default;
        RenderComponent(std::shared_ptr<Material> mat) : material(std::move(mat)) {}
    };

    struct MeshComponent {
        std::shared_ptr<class Mesh> mesh;
    };

    struct CameraComponent {
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        float yaw;
        float pitch;
        float fov;
        
        CameraComponent() : front(0,0,-1), up(0,1,0), right(1,0,0), yaw(-90.0f), pitch(0.0f), fov(45.0f) {}
    };

    struct PointLightComponent {
        glm::vec3 color{1.0f, 1.0f, 1.0f};
        float intensity{1.0f};
        float radius{10.0f};

        PointLightComponent() = default;
        PointLightComponent(const glm::vec3& c, float i, float r)
            : color(c), intensity(i), radius(r) {}
    };

    struct DirectionalLightComponent {
        glm::vec3 color{1.0f, 1.0f, 1.0f};
        float intensity{1.0f};

        DirectionalLightComponent() = default;
        DirectionalLightComponent(const glm::vec3& c, float i)
            : color(c), intensity(i) {}
    };

}
