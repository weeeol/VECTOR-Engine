#include "Engine/Graphics/ParticleSystem.hpp"
#include <cstdlib>

namespace VECTOR {

    ParticleEmitter::ParticleEmitter(int maxParticles) : m_PoolIndex(maxParticles - 1) {
        m_Particles.resize(maxParticles);
        for (auto& p : m_Particles) {
            p.active = false;
        }
    }

    void ParticleEmitter::Emit(float x, float y, int count, Uint8 r, Uint8 g, Uint8 b, float speed, float life) {
        for (int i = 0; i < count; i++) {
            // Find next inactive particle
            bool found = false;
            for (int j = 0; j < m_Particles.size(); j++) {
                if (!m_Particles[m_PoolIndex].active) {
                    found = true;
                    break;
                }
                m_PoolIndex = (m_PoolIndex - 1 + m_Particles.size()) % m_Particles.size();
            }

            if (!found) return; // Pool full

            Particle& p = m_Particles[m_PoolIndex];
            p.active = true;
            p.x = x;
            p.y = y;

            // Random direction
            float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
            float randomSpeed = speed * (0.5f + (float)(rand() % 100) / 200.0f); // 0.5 to 1.0 multiplier

            p.vx = std::cos(angle) * randomSpeed;
            p.vy = std::sin(angle) * randomSpeed;

            p.life = life;
            p.maxLife = life;
            p.r = r;
            p.g = g;
            p.b = b;

            m_PoolIndex = (m_PoolIndex - 1 + m_Particles.size()) % m_Particles.size();
        }
    }

    void ParticleEmitter::Update(float deltaTime) {
        for (auto& p : m_Particles) {
            if (!p.active) continue;

            p.life -= deltaTime;
            if (p.life <= 0.0f) {
                p.active = false;
                continue;
            }

            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
        }
    }

    void ParticleEmitter::Render(Renderer* renderer, int offsetX, int offsetY) {
        for (auto& p : m_Particles) {
            if (!p.active) continue;

            // Fade alpha based on life remaining
            Uint8 alpha = (Uint8)(255.0f * (p.life / p.maxLife));
            
            // Draw as a small 4x4 rect
            renderer->DrawRect((int)p.x + offsetX, (int)p.y + offsetY, 4, 4, p.r, p.g, p.b, alpha);
        }
    }

} // namespace VECTOR
