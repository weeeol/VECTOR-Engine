#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include <vector>

namespace VECTOR {

    struct Particle {
        float x, y;
        float vx, vy;
        float life;
        float maxLife;
        Uint8 r, g, b;
        bool active;
    };

    class ParticleEmitter {
    public:
        ParticleEmitter(int maxParticles = 100);
        ~ParticleEmitter() = default;

        void Emit(float x, float y, int count, Uint8 r, Uint8 g, Uint8 b, float speed = 100.0f, float life = 1.0f);
        
        void Update(float deltaTime);
        void Render(Renderer* renderer, int offsetX = 0, int offsetY = 0);

    private:
        std::vector<Particle> m_Particles;
        int m_PoolIndex;
    };

} // namespace VECTOR
