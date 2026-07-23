#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "Engine/Graphics/Material.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include <future>

namespace VECTOR {
    class Model;
    class Animation;

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
        std::shared_ptr<Shader> GetShader(const std::string& name);

        // Loads and caches a Texture2D from a file
        std::shared_ptr<Texture2D> LoadTexture2D(const std::string& name, const std::string& filepath);
        std::shared_ptr<Texture2D> GetTexture2D(const std::string& name);

        // Loads and caches a Material from a .vmat file
        std::shared_ptr<Material> LoadMaterial(const std::string& name, const std::string& filepath);
        std::shared_ptr<Material> GetMaterial(const std::string& name);

        // Loads and caches a Model from a file
        std::shared_ptr<Model> LoadModel(const std::string& name, const std::string& filepath);
        std::shared_ptr<Model> GetModel(const std::string& name);
        
        // Loads a Model asynchronously using the JobSystem
        std::future<std::shared_ptr<Model>> LoadModelAsync(const std::string& name, const std::string& filepath);

        // Loads and caches an Animation from a file (assumes the model also has to be parsed)
        std::shared_ptr<Animation> LoadAnimation(const std::string& name, const std::string& filepath, std::shared_ptr<Model> targetModel);
        std::shared_ptr<Animation> GetAnimation(const std::string& name);

    private:
        ResourceManager() = default;
        ~ResourceManager() = default;

        std::unordered_map<std::string, TTF_Font*> m_Fonts;
        std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
        std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_Textures;
        std::unordered_map<std::string, std::shared_ptr<Material>> m_Materials;
        std::unordered_map<std::string, std::shared_ptr<Model>> m_Models;
        std::unordered_map<std::string, std::shared_ptr<Animation>> m_Animations;
        std::mutex m_ResourceMutex;
    };

} // namespace VECTOR
