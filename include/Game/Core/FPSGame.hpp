#pragma once

#include "Engine/Core/Application.hpp"
#include <memory>

namespace Game {

    class FPSGame : public VECTOR::Application {
    public:
        FPSGame(const std::string& title, int width, int height);
        ~FPSGame();

    protected:
        void OnInit() override;
        void Update(float deltaTime) override;
        void Render() override;

    private:
        void SetupEventSubscriptions();
    };

} // namespace Game
