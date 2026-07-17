#pragma once

#include <string>

namespace Game {

    class SaveManager {
    public:
        static void LoadData(int& outHighScorePlayer, int& outHighScoreAI, float& outVolume);
        static void SaveData(int highScorePlayer, int highScoreAI, float volume);

        // Settings persistence
        static void LoadSettings(float& outVolume, bool& outBorderless, int& outResWidth, int& outResHeight);
        static void SaveSettings(float volume, bool borderless, int resWidth, int resHeight);

    private:
        static const std::string s_SaveFilePath;
        static const std::string s_SettingsFilePath;
    };

} // namespace Game
