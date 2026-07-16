#pragma once

namespace VECTOR {

    class Renderer;

    class Scene {
    public:
        virtual ~Scene() = default;

        virtual void OnEnter() {}
        virtual void OnExit() {}

        virtual void Update(float deltaTime) = 0;
        virtual void Render(Renderer* renderer) = 0;
    };

} // namespace VECTOR
