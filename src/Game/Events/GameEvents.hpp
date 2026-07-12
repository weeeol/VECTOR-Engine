#pragma once

#include "Engine/Events/Event.hpp"

namespace Game {

    struct CollisionEvent : public VECTOR::Event {
        // Can hold info about collision, e.g. velocity, position, etc.
        CollisionEvent() = default;
    };

    struct ScoreEvent : public VECTOR::Event {
        int player; // 1 or 2
        ScoreEvent(int playerNum) : player(playerNum) {}
    };

} // namespace Game
