#pragma once

#include <string>

namespace Game {

    class SaveManager {
    public:
        static void LoadData(int& outHighScorePlayer, int& outHighScoreAI, float& outVolume);
        static void SaveData(int highScorePlayer, int highScoreAI, float volume);

    private:
        static const std::string s_SaveFilePath;
    };

} // namespace Game
