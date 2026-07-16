#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stdint.h>

class btRigidBody;

namespace VECTOR {
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
        std::shared_ptr<class Shader> shader;
        std::shared_ptr<class Texture2D> texture;
        glm::vec4 color;

        RenderComponent() : color(1.0f) {}
        RenderComponent(const glm::vec4& c) : color(c) {}
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
}
