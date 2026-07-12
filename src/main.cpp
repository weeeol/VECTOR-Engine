#include "Game/Core/PongGame.hpp"

int main(int argc, char* argv[]) {
    // We instantiate the game, which is an VECTOR::Application
    Game::PongGame game("Pong Game", 1280, 720);

    // Run the game loop
    game.Run();

    return 0;
}
