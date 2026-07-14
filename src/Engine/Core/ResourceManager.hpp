#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "Engine/Graphics/Shader.hpp"

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

        // Loads and caches a shader from files
        std::shared_ptr<Shader> LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
        
        // Retrieves a previously loaded shader by name
        std::shared_ptr<Shader> GetShader(const std::string& name);

    private:
        ResourceManager() = default;
        ~ResourceManager() = default;

        // Key for fonts is "path_size" to differentiate same font, different sizes
        std::unordered_map<std::string, TTF_Font*> m_Fonts;

        // Cache for loaded shaders by name
        std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
    };

} // namespace VECTOR
