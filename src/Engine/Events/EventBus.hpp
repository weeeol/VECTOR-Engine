#pragma once

#include "Engine/Events/Event.hpp"
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

namespace VECTOR {

    class EventBus {
    public:
        static EventBus& Get() {
            static EventBus instance;
            return instance;
        }

        EventBus(const EventBus&) = delete;
        EventBus& operator=(const EventBus&) = delete;

        template<typename T>
        using EventCallback = std::function<void(const T&)>;

        // Subscribe to an event
        template<typename T>
        uint32_t Subscribe(EventCallback<T> callback) {
            std::type_index typeId = typeid(T);
            
            auto wrapper = [callback](const Event& event) {
                callback(static_cast<const T&>(event));
            };
            
            uint32_t id = ++m_NextId;
            m_Subscribers[typeId].push_back({id, wrapper});
            return id;
        }

        // Unsubscribe from an event
        template<typename T>
        void Unsubscribe(uint32_t id) {
            std::type_index typeId = typeid(T);
            auto it = m_Subscribers.find(typeId);
            if (it != m_Subscribers.end()) {
                auto& subs = it->second;
                for (auto vIt = subs.begin(); vIt != subs.end(); ++vIt) {
                    if (vIt->id == id) {
                        subs.erase(vIt);
                        break;
                    }
                }
            }
        }

        // Publish an event
        template<typename T, typename... Args>
        void Publish(Args&&... args) {
            T event(std::forward<Args>(args)...);
            std::type_index typeId = typeid(T);

            if (m_Subscribers.find(typeId) != m_Subscribers.end()) {
                for (auto& record : m_Subscribers[typeId]) {
                    record.callback(event);
                }
            }
        }

    private:
        EventBus() = default;
        ~EventBus() = default;

        using InternalCallback = std::function<void(const Event&)>;
        struct SubRecord {
            uint32_t id;
            InternalCallback callback;
        };
        std::unordered_map<std::type_index, std::vector<SubRecord>> m_Subscribers;
        uint32_t m_NextId = 0;
    };

} // namespace VECTOR
