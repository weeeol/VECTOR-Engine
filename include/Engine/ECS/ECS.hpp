#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <cstdint>
#include <cassert>
#include <algorithm>

#include <entt/entt.hpp>

namespace VECTOR {
    using Entity = entt::entity;

    class Registry {
    public:
        entt::registry m_Registry;

        Entity CreateEntity() { return m_Registry.create(); }
        void DestroyEntity(Entity e) { m_Registry.destroy(e); }
        void Clear() { m_Registry.clear(); }

        template<typename T>
        void AddComponent(Entity e, T component) {
            m_Registry.emplace_or_replace<T>(e, std::move(component));
        }

        template<typename T>
        void RemoveComponent(Entity e) { m_Registry.remove<T>(e); }

        template<typename T>
        T& GetComponent(Entity e) { return m_Registry.get<T>(e); }

        template<typename T>
        bool HasComponent(Entity e) { return m_Registry.all_of<T>(e); }

        template<typename T, typename... Rest, typename Func>
        void View(Func func) {
            auto view = m_Registry.view<T, Rest...>();
            // EnTT views iterate backwards by default. Buffer and iterate reverse 
            // to process in creation order, matching the original custom ECS behavior.
            std::vector<Entity> entities;
            for (auto entity : view) {
                entities.push_back(entity);
            }
            for (auto it = entities.rbegin(); it != entities.rend(); ++it) {
                func(*it);
            }
        }

        template<typename T>
        void RegisterComponent() { /* EnTT doesn't need explicit registration */ }

        // Maintain compatibility with SceneSerializer.cpp
        std::vector<Entity> GetActiveEntities() const {
            std::vector<Entity> active;
            auto* storage = m_Registry.storage<Entity>();
            if (storage) {
                for (auto entity : *storage) {
                    active.push_back(entity);
                }
            }
            return active;
        }
    };
}
