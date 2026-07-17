#include "Engine/Core/ResourceManager.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/Shader.hpp"
#include <fstream>
#include <sstream>

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
                VECTOR_LOG_WARN(std::string("Failed to load font: ") + fontPath + "! SDL_Error: " + SDL_GetError());
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

    std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(const std::string& name, const std::string& filepath) {
        if (m_Textures.find(name) != m_Textures.end()) {
            return m_Textures[name];
        }

        auto texture = Texture2D::Create(filepath);
        if (texture) {
            m_Textures[name] = texture;
        } else {
            VECTOR_LOG_ERROR("Failed to load Texture2D: " + filepath);
        }
        return texture;
    }

    std::shared_ptr<Texture2D> ResourceManager::GetTexture2D(const std::string& name) {
        if (m_Textures.find(name) != m_Textures.end()) {
            return m_Textures[name];
        }
        return nullptr;
    }

    std::shared_ptr<Material> ResourceManager::LoadMaterial(const std::string& name, const std::string& filepath) {
        if (m_Materials.find(name) != m_Materials.end()) {
            return m_Materials[name];
        }

        std::ifstream file(filepath);
        if (!file.is_open()) {
            VECTOR_LOG_ERROR("Failed to open material file: " + filepath);
            return nullptr;
        }

        auto mat = std::make_shared<Material>();
        std::string line;
        while (std::getline(file, line)) {
            // Very simple parser: key = value
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;

            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            // Trim whitespace and \r
            key.erase(0, key.find_first_not_of(" \t\r"));
            key.erase(key.find_last_not_of(" \t\r") + 1);
            value.erase(0, value.find_first_not_of(" \t\r"));
            value.erase(value.find_last_not_of(" \t\r") + 1);

            if (key == "shader") {
                mat->shader = GetShader(value);
                if (!mat->shader) {
                    VECTOR_LOG_WARN("Shader not found for material: " + value);
                }
            } else if (key == "albedoColor") {
                std::istringstream iss(value);
                float r, g, b, a;
                if (iss >> r >> g >> b >> a) {
                    mat->albedoColor = glm::vec4(r, g, b, a);
                }
            } else if (key == "roughness") {
                mat->roughness = std::stof(value);
            } else if (key == "metallic") {
                mat->metallic = std::stof(value);
            } else if (key == "specularStrength") {
                mat->roughness = 1.0f - std::stof(value);
            } else if (key == "shininess") {
                float shin = std::stof(value);
                mat->roughness = std::sqrt(2.0f / (shin + 2.0f));
            } else if (key == "isUnlit") {
                mat->isUnlit = (value == "true" || value == "1");
            } else if (key == "diffuseTexture") {
                // If it's a new texture, load it, else get it
                auto tex = GetTexture2D(value);
                if (!tex) {
                    tex = LoadTexture2D(value, value); // use path as name
                }
                mat->albedoTexture = tex;
            }
        }

        m_Materials[name] = mat;
        return mat;
    }

    std::shared_ptr<Material> ResourceManager::GetMaterial(const std::string& name) {
        if (m_Materials.find(name) != m_Materials.end()) {
            return m_Materials[name];
        }
        return nullptr;
    }

} // namespace VECTOR
