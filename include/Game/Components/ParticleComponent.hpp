#pragma once
#include <glm/glm.hpp>

namespace Game {

    struct ParticleComponent {
        glm::vec3 velocity;
        float lifetime;
        float maxLifetime;
    };

} // namespace Game
