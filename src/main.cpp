#include <SDL.h>
#include "Engine/Core/Application.hpp"
#include <memory>

int main(int argc, char* argv[]) {
    // Instantiate the application via the client-defined entry point
    std::unique_ptr<VECTOR::Application> app(VECTOR::CreateApplication());

    // Run the game loop
    app->Run();

    return 0;
}
