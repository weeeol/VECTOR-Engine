#include "Engine/Core/ResourceManager.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    void ResourceManager::Initialize() {
        VECTOR_LOG_INFO("ResourceManager initialized successfully.");
    }

    void ResourceManager::Shutdown() {
        for (auto& pair : m_Fonts) {
            if (pair.second) {
                TTF_CloseFont(pair.second);
            }
        }
        m_Fonts.clear();
        m_Shaders.clear();
    }

    TTF_Font* ResourceManager::GetFont(const std::string& fontPath, int fontSize) {
        std::string key = fontPath + "_" + std::to_string(fontSize);

        if (m_Fonts.find(key) == m_Fonts.end()) {
            TTF_Font* font = TTF_OpenFont(fontPath.c_str(), fontSize);
            if (!font) {
                VECTOR_LOG_WARN(std::string("Failed to load font: ") + fontPath + "! TTF_Error: " + TTF_GetError());
                return nullptr;
            }
            m_Fonts[key] = font;
        }

        return m_Fonts[key];
    }

    std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
        if (m_Shaders.find(name) != m_Shaders.end()) {
            return m_Shaders[name];
        }

        auto shader = Shader::CreateFromFile(vertexPath, fragmentPath);
        if (shader) {
            m_Shaders[name] = shader;
        } else {
            VECTOR_LOG_ERROR("Failed to load shader: " + name);
        }

        return shader;
    }

    std::shared_ptr<Shader> ResourceManager::GetShader(const std::string& name) {
        if (m_Shaders.find(name) != m_Shaders.end()) {
            return m_Shaders[name];
        }
        return nullptr;
    }

} // namespace VECTOR
