#pragma once

#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include <unordered_map>

#include <GL/glew.h>
#include <SDL_opengl.h>
#include <glm/glm.hpp>
#include <memory>
#include "Shader.hpp"
#include "Mesh.hpp"
#include "Texture2D.hpp"

namespace VECTOR {

    class Renderer {
    public:
        Renderer();
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        bool Initialize(const std::string& title, int width, int height);
        void Shutdown();

        void Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        void Present();

        void SetViewProjection(const glm::mat4& view, const glm::mat4& projection);
        void DrawMesh(const class Mesh* mesh, const class Shader* shader, const class Texture2D* texture, const glm::mat4& model, const glm::vec4& color);
        
        // 2D API
        void DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        void DrawText(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b, int fontSize = 24);

        SDL_Window* GetWindow() const { return m_Window; }

    private:
        SDL_Window* m_Window;
        SDL_GLContext m_GLContext;
        std::shared_ptr<Shader> m_DefaultShader;
        glm::mat4 m_ViewMatrix;
        glm::mat4 m_ProjectionMatrix;
        
        // 2D Rendering
        std::shared_ptr<Shader> m_2DShader;
        unsigned int m_QuadVAO, m_QuadVBO;
        int m_Width, m_Height;
        struct TextTexture {
            unsigned int id;
            int w, h;
        };
        std::unordered_map<std::string, TextTexture> m_TextTextureCache;
    };

} // namespace VECTOR
