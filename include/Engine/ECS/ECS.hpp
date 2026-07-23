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

    class IComponentArray {
    public:
        virtual ~IComponentArray() = default;
        virtual void EntityDestroyed(Entity entity) = 0;
        virtual void Clear() = 0;
    };

    template<typename T>
    class ComponentArray : public IComponentArray {
    public:
        ComponentArray() = default;

        void InsertData(Entity entity, T component) {
            if (entity >= m_EntityToIndex.size()) {
                m_EntityToIndex.resize(entity + 1, (size_t)-1);
            }
            
            assert(m_EntityToIndex[entity] == (size_t)-1 && "Component added to same entity more than once.");
            
            size_t newIndex = m_ComponentArray.size();
            m_EntityToIndex[entity] = newIndex;
            m_IndexToEntity.push_back(entity);
            m_ComponentArray.push_back(std::move(component));
        }

        void RemoveData(Entity entity) {
            assert(entity < m_EntityToIndex.size() && m_EntityToIndex[entity] != (size_t)-1 && "Removing non-existent component.");
            
            size_t indexOfRemovedEntity = m_EntityToIndex[entity];
            size_t indexOfLastElement = m_ComponentArray.size() - 1;
            
            if (indexOfRemovedEntity != indexOfLastElement) {
                m_ComponentArray[indexOfRemovedEntity] = std::move(m_ComponentArray[indexOfLastElement]);

                Entity entityOfLastElement = m_IndexToEntity[indexOfLastElement];
                m_EntityToIndex[entityOfLastElement] = indexOfRemovedEntity;
                m_IndexToEntity[indexOfRemovedEntity] = entityOfLastElement;
            }

            m_EntityToIndex[entity] = (size_t)-1;

            m_ComponentArray.pop_back();
            m_IndexToEntity.pop_back();
        }

        T& GetData(Entity entity) {
            assert(entity < m_EntityToIndex.size() && m_EntityToIndex[entity] != (size_t)-1 && "Retrieving non-existent component.");
            return m_ComponentArray[m_EntityToIndex[entity]];
        }

        void EntityDestroyed(Entity entity) override {
            if (entity < m_EntityToIndex.size() && m_EntityToIndex[entity] != (size_t)-1) {
                RemoveData(entity);
            }
        }
        
        bool HasData(Entity entity) const {
            return entity < m_EntityToIndex.size() && m_EntityToIndex[entity] != (size_t)-1;
        }

        void Clear() override {
            m_ComponentArray.clear();
            m_IndexToEntity.clear();
            m_EntityToIndex.clear();
        }

        const Entity* GetDenseEntityArray() const {
            return m_IndexToEntity.data();
        }

        size_t GetSize() const {
            return m_ComponentArray.size();
        }

    private:
        std::vector<T> m_ComponentArray;
        std::vector<size_t> m_EntityToIndex;
        std::vector<Entity> m_IndexToEntity;
    };

    class Registry {
    public:
        Registry() : m_NextEntity(0) {}
        
        Entity CreateEntity() {
            Entity id;
            if (!m_FreeEntities.empty()) {
                id = m_FreeEntities.back();
                m_FreeEntities.pop_back();
            } else {
                id = m_NextEntity++;
            }
            m_ActiveEntities.push_back(id);
            return id;
        }
        
        void DestroyEntity(Entity entity) {
            for (auto const& pair : m_ComponentArrays) {
                pair.second->EntityDestroyed(entity);
            }
            auto it = std::find(m_ActiveEntities.begin(), m_ActiveEntities.end(), entity);
            if (it != m_ActiveEntities.end()) {
                std::swap(*it, m_ActiveEntities.back());
                m_ActiveEntities.pop_back();
            }
            m_FreeEntities.push_back(entity);
        }

        void Clear() {
            m_ActiveEntities.clear();
            m_FreeEntities.clear();
            for (auto const& pair : m_ComponentArrays) {
                pair.second->Clear();
            }
            m_NextEntity = 0;
        }

        const std::vector<Entity>& GetActiveEntities() const {
            return m_ActiveEntities;
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
        std::vector<Entity> m_FreeEntities;
        std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> m_ComponentArrays;
    };
}
