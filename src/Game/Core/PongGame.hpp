#pragma once

#include "Engine/Core/Application.hpp"
#include <memory>

namespace Game {

    class PongGame : public VECTOR::Application {
    public:
        PongGame(const std::string& title, int width, int height);
        ~PongGame();

    protected:
        void OnInit() override;
        void Update(float deltaTime) override;
        void Render() override;

    private:
        void SetupEventSubscriptions();
    };

} // namespace Game
