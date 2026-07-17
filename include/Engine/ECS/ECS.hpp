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
        virtual void Clear() = 0;
    };

    template<typename T>
    class ComponentArray : public IComponentArray {
    public:
        ComponentArray() {
            std::fill_n(m_EntityToIndex, MAX_ENTITIES, (size_t)-1);
        }

        void InsertData(Entity entity, T component) {
            assert(entity < MAX_ENTITIES && m_EntityToIndex[entity] == (size_t)-1 && "Component added to same entity more than once or entity out of bounds.");
            size_t newIndex = m_Size;
            m_EntityToIndex[entity] = newIndex;
            m_IndexToEntity[newIndex] = entity;
            m_ComponentArray[newIndex] = component;
            m_Size++;
        }

        void RemoveData(Entity entity) {
            assert(entity < MAX_ENTITIES && m_EntityToIndex[entity] != (size_t)-1 && "Removing non-existent component.");
            size_t indexOfRemovedEntity = m_EntityToIndex[entity];
            size_t indexOfLastElement = m_Size - 1;
            
            if (indexOfRemovedEntity != indexOfLastElement) {
                m_ComponentArray[indexOfRemovedEntity] = std::move(m_ComponentArray[indexOfLastElement]);

                Entity entityOfLastElement = m_IndexToEntity[indexOfLastElement];
                m_EntityToIndex[entityOfLastElement] = indexOfRemovedEntity;
                m_IndexToEntity[indexOfRemovedEntity] = entityOfLastElement;
            }

            m_EntityToIndex[entity] = (size_t)-1;
            m_Size--;
        }

        T& GetData(Entity entity) {
            assert(entity < MAX_ENTITIES && m_EntityToIndex[entity] != (size_t)-1 && "Retrieving non-existent component.");
            return m_ComponentArray[m_EntityToIndex[entity]];
        }

        void EntityDestroyed(Entity entity) override {
            if (entity < MAX_ENTITIES && m_EntityToIndex[entity] != (size_t)-1) {
                RemoveData(entity);
            }
        }
        
        bool HasData(Entity entity) const {
            return entity < MAX_ENTITIES && m_EntityToIndex[entity] != (size_t)-1;
        }

        void Clear() override {
            std::fill_n(m_EntityToIndex, MAX_ENTITIES, (size_t)-1);
            m_Size = 0;
        }

        const Entity* GetDenseEntityArray() const {
            return m_IndexToEntity;
        }

        size_t GetSize() const {
            return m_Size;
        }

    private:
        T m_ComponentArray[MAX_ENTITIES];
        size_t m_EntityToIndex[MAX_ENTITIES];
        Entity m_IndexToEntity[MAX_ENTITIES];
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
            if (it != m_ActiveEntities.end()) {
                // Swap and pop for faster removal
                std::swap(*it, m_ActiveEntities.back());
                m_ActiveEntities.pop_back();
            }
        }

        void Clear() {
            m_ActiveEntities.clear();
            for (auto const& pair : m_ComponentArrays) {
                pair.second->Clear();
            }
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
            GetComponentArray<T>()->InsertData(entity, std::move(component));
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
            std::type_index typeName = std::type_index(typeid(T));
            if (m_ComponentArrays.find(typeName) == m_ComponentArrays.end()) return false;
            return GetComponentArray<T>()->HasData(entity);
        }

        template<typename T, typename... Rest, typename Func>
        void View(Func func) {
            auto firstArray = GetComponentArray<T>();
            const Entity* entities = firstArray->GetDenseEntityArray();
            size_t size = firstArray->GetSize();
            
            for (size_t i = 0; i < size; ++i) {
                Entity entity = entities[i];
                if constexpr (sizeof...(Rest) > 0) {
                    if ((HasComponent<Rest>(entity) && ...)) {
                        func(entity);
                    }
                } else {
                    func(entity);
                }
            }
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
