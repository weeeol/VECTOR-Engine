#include "Engine/Core/Application.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include <memory>
#include <string>
#include <iostream>

int main(int argc, char* argv[]) {
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--opengl") {
            VECTOR::RendererAPI::SetAPI(VECTOR::RendererAPI::API::OpenGL);
            std::cout << "Command Line: Forcing OpenGL Backend" << std::endl;
        } else if (arg == "--vulkan") {
            VECTOR::RendererAPI::SetAPI(VECTOR::RendererAPI::API::Vulkan);
            std::cout << "Command Line: Forcing Vulkan Backend" << std::endl;
        }
    }

    // Instantiate the application via the client-defined entry point
    std::unique_ptr<VECTOR::Application> app(VECTOR::CreateApplication());

    // Run the game loop
    app->Run();

    return 0;
}
