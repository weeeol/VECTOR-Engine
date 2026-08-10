#include "Game/Systems/ParticleSystem.hpp"
#include "Game/Components/ParticleComponent.hpp"
#include "Engine/ECS/Components.hpp"

namespace Game {

    void ParticleSystem::Update(VECTOR::Registry& registry, float deltaTime) {
        // Collect entities to destroy
        std::vector<VECTOR::Entity> toDestroy;

        registry.View<VECTOR::TransformComponent, ParticleComponent>([&](VECTOR::Entity entity) {
            auto& t = registry.GetComponent<VECTOR::TransformComponent>(entity);
            auto& p = registry.GetComponent<ParticleComponent>(entity);

            // Integrate velocity
            t.position += p.velocity * deltaTime;
            
            // Add some gravity
            p.velocity.y -= 9.81f * 1.5f * deltaTime; // Slightly heavier gravity for effect

            // Shrink over time
            float scaleRatio = p.lifetime / p.maxLifetime;
            t.scale = glm::vec3(scaleRatio) * 0.2f; // Assuming original scale was ~0.2

            p.lifetime -= deltaTime;
            if (p.lifetime <= 0.0f) {
                toDestroy.push_back(entity);
            }
        });

        for (auto entity : toDestroy) {
            registry.DestroyEntity(entity);
        }
    }

} // namespace Game
