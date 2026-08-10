#pragma once

#include "Engine/Events/Event.hpp"

namespace Game {

    struct CollisionEvent : public VECTOR::Event {
        glm::vec3 position;
        CollisionEvent(glm::vec3 pos) : position(pos) {}
    };

    struct ScoreEvent : public VECTOR::Event {
        int player; // 1 or 2
        ScoreEvent(int playerNum) : player(playerNum) {}
    };

} // namespace Game
