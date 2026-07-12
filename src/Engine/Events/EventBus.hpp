#pragma once

#include "Engine/Events/Event.hpp"
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <memory>

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
        void Subscribe(EventCallback<T> callback) {
            std::type_index typeId = typeid(T);
            
            auto wrapper = [callback](const Event& event) {
                callback(static_cast<const T&>(event));
            };
            
            m_Subscribers[typeId].push_back(wrapper);
        }

        // Publish an event
        template<typename T, typename... Args>
        void Publish(Args&&... args) {
            T event(std::forward<Args>(args)...);
            std::type_index typeId = typeid(T);

            if (m_Subscribers.find(typeId) != m_Subscribers.end()) {
                for (auto& callback : m_Subscribers[typeId]) {
                    callback(event);
                }
            }
        }

    private:
        EventBus() = default;
        ~EventBus() = default;

        using InternalCallback = std::function<void(const Event&)>;
        std::unordered_map<std::type_index, std::vector<InternalCallback>> m_Subscribers;
    };

} // namespace VECTOR
