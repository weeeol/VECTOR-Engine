#pragma once

#include <box2d/box2d.h>

namespace VECTOR {
    class PhysicsUtils {
    public:
        static b2BodyId CreateBox(b2WorldId worldId, float x, float y, float width, float height, 
                                  b2BodyType type, float density, float friction, float restitution, 
                                  bool isSensor, void* userData);
    };
}
