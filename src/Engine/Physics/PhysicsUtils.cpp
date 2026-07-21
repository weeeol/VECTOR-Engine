#include "Engine/Physics/PhysicsUtils.hpp"
#include "Engine/Physics/Box2DPhysicsSystem.hpp"

namespace VECTOR {

    b2BodyId PhysicsUtils::CreateBox(b2WorldId worldId, float x, float y, float width, float height, 
                                     b2BodyType type, float density, float friction, float restitution, 
                                     bool isSensor, void* userData) 
    {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = type;
        bodyDef.position = b2Vec2{x / VECTOR::PIXELS_PER_METER, y / VECTOR::PIXELS_PER_METER};
        bodyDef.userData = userData;
        bodyDef.fixedRotation = true;
        bodyDef.isBullet = (type == b2_dynamicBody);
        b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);

        b2Polygon box = b2MakeBox((width / 2.0f) / VECTOR::PIXELS_PER_METER, (height / 2.0f) / VECTOR::PIXELS_PER_METER);

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = density;
        shapeDef.material.friction = friction;
        shapeDef.material.restitution = restitution;
        shapeDef.isSensor = isSensor;
        shapeDef.enableContactEvents = true; 
        shapeDef.enableSensorEvents = true;

        b2CreatePolygonShape(bodyId, &shapeDef, &box);
        return bodyId;
    }

}
