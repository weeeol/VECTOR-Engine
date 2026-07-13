#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <cstdint>
#include <cassert>
#include <algorithm>

namespace VECTOR {
    using Entity = uint32_t;
    const Entity MAX_ENTITIES = 5000;

    class IComponentArray {
    public:
        virtual ~IComponentArray() = default;
        virtual void EntityDestroyed(Entity entity) = 0;
    };

    template<typename T>
    class ComponentArray : public IComponentArray {
    public:
        void InsertData(Entity entity, T component) {
            assert(m_EntityToIndexMap.find(entity) == m_EntityToIndexMap.end() && "Component added to same entity more than once.");
            size_t newIndex = m_Size;
            m_EntityToIndexMap[entity] = newIndex;
            m_IndexToEntityMap[newIndex] = entity;
            m_ComponentArray[newIndex] = component;
            m_Size++;
        }

        void RemoveData(Entity entity) {
            assert(m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end() && "Removing non-existent component.");
            size_t indexOfRemovedEntity = m_EntityToIndexMap[entity];
            size_t indexOfLastElement = m_Size - 1;
            m_ComponentArray[indexOfRemovedEntity] = m_ComponentArray[indexOfLastElement];

            Entity entityOfLastElement = m_IndexToEntityMap[indexOfLastElement];
            m_EntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
            m_IndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

            m_EntityToIndexMap.erase(entity);
            m_IndexToEntityMap.erase(indexOfLastElement);
            m_Size--;
        }

        T& GetData(Entity entity) {
            assert(m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end() && "Retrieving non-existent component.");
            return m_ComponentArray[m_EntityToIndexMap[entity]];
        }

        void EntityDestroyed(Entity entity) override {
            if (m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end()) {
                RemoveData(entity);
            }
        }
        
        bool HasData(Entity entity) const {
            return m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end();
        }

    private:
        T m_ComponentArray[MAX_ENTITIES];
        std::unordered_map<Entity, size_t> m_EntityToIndexMap;
        std::unordered_map<size_t, Entity> m_IndexToEntityMap;
        size_t m_Size = 0;
    };

    class Registry {
    public:
        Registry() : m_NextEntity(0) {}
        
        Entity CreateEntity() {
            assert(m_NextEntity < MAX_ENTITIES && "Too many entities in existence.");
            Entity id = m_NextEntity++;
            m_ActiveEntities.push_back(id);
            return id;
        }
        
        void DestroyEntity(Entity entity) {
            for (auto const& pair : m_ComponentArrays) {
                pair.second->EntityDestroyed(entity);
            }
            auto it = std::find(m_ActiveEntities.begin(), m_ActiveEntities.end(), entity);
            if (it != m_ActiveEntities.end()) m_ActiveEntities.erase(it);
        }

        void Clear() {
            m_ActiveEntities.clear();
            m_ComponentArrays.clear();
            m_NextEntity = 0;
        }

        template<typename T>
        void RegisterComponent() {
            std::type_index typeName = std::type_index(typeid(T));
            assert(m_ComponentArrays.find(typeName) == m_ComponentArrays.end() && "Registering component type more than once.");
            m_ComponentArrays[typeName] = std::make_shared<ComponentArray<T>>();
        }

        template<typename T>
        void AddComponent(Entity entity, T component) {
            GetComponentArray<T>()->InsertData(entity, component);
        }

        template<typename T>
        void RemoveComponent(Entity entity) {
            GetComponentArray<T>()->RemoveData(entity);
        }

        template<typename T>
        T& GetComponent(Entity entity) {
            return GetComponentArray<T>()->GetData(entity);
        }

        template<typename T>
        bool HasComponent(Entity entity) {
            // Check if registered
            std::type_index typeName = std::type_index(typeid(T));
            if (m_ComponentArrays.find(typeName) == m_ComponentArrays.end()) return false;
            return GetComponentArray<T>()->HasData(entity);
        }

        template<typename... Components>
        std::vector<Entity> View() {
            std::vector<Entity> result;
            for (Entity entity : m_ActiveEntities) {
                if ((HasComponent<Components>(entity) && ...)) {
                    result.push_back(entity);
                }
            }
            return result;
        }

    private:
        template<typename T>
        std::shared_ptr<ComponentArray<T>> GetComponentArray() {
            std::type_index typeName = std::type_index(typeid(T));
            assert(m_ComponentArrays.find(typeName) != m_ComponentArrays.end() && "Component not registered before use.");
            return std::static_pointer_cast<ComponentArray<T>>(m_ComponentArrays[typeName]);
        }

        Entity m_NextEntity;
        std::vector<Entity> m_ActiveEntities;
        std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> m_ComponentArrays;
    };
}
