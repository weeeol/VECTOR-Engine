#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <unordered_map>

namespace VECTOR {

    class ResourceManager {
    public:
        static ResourceManager& Get() {
            static ResourceManager instance;
            return instance;
        }

        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        void Initialize();
        void Shutdown();

        // Caches and returns a font
        TTF_Font* GetFont(const std::string& fontPath, int fontSize);

    private:
        ResourceManager() = default;
        ~ResourceManager() = default;

        // Key for fonts is "path_size" to differentiate same font, different sizes
        std::unordered_map<std::string, TTF_Font*> m_Fonts;
    };

} // namespace VECTOR
