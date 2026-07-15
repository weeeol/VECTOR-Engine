#include "Game/Core/SaveManager.hpp"
#include <fstream>
#include <sstream>

namespace Game {

    const std::string SaveManager::s_SaveFilePath = "save.dat";

    void SaveManager::LoadData(int& outHighScorePlayer, int& outHighScoreAI, float& outVolume) {
        // Defaults
        outHighScorePlayer = 0;
        outHighScoreAI = 0;
        outVolume = 0.5f;

        std::ifstream file(s_SaveFilePath);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                std::istringstream iss(line);
                std::string key;
                if (std::getline(iss, key, '=')) {
                    std::string value;
                    if (std::getline(iss, value)) {
                        if (key == "HighScorePlayer") {
                            try { outHighScorePlayer = std::stoi(value); } catch (...) {}
                        } else if (key == "HighScoreAI") {
                            try { outHighScoreAI = std::stoi(value); } catch (...) {}
                        } else if (key == "Volume") {
                            try { outVolume = std::stof(value); } catch (...) {}
                        }
                    }
                }
            }
            file.close();
        }
    }

    void SaveManager::SaveData(int highScorePlayer, int highScoreAI, float volume) {
        std::ofstream file(s_SaveFilePath);
        if (file.is_open()) {
            file << "HighScorePlayer=" << highScorePlayer << "\n";
            file << "HighScoreAI=" << highScoreAI << "\n";
            file << "Volume=" << volume << "\n";
            file.close();
        }
    }

} // namespace Game
