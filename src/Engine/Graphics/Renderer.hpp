#pragma once

#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include <unordered_map>

#include <GL/glew.h>
#include <SDL_opengl.h>
#include <glm/glm.hpp>

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
        void DrawMesh(unsigned int VAO, int indexCount, const glm::mat4& model, const glm::vec3& color);
        
        // 2D API
        void DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        void DrawText(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b, int fontSize = 24);

        SDL_Window* GetWindow() const { return m_Window; }

    private:
        SDL_Window* m_Window;
        SDL_GLContext m_GLContext;
        
        unsigned int m_ShaderProgram;
        glm::mat4 m_ViewMatrix;
        glm::mat4 m_ProjectionMatrix;
        
        // 2D Rendering
        unsigned int m_2DShaderProgram;
        unsigned int m_QuadVAO, m_QuadVBO;
        int m_Width, m_Height;
        std::unordered_map<int, void*> m_Fonts; // Store TTF_Font* as void* to avoid exposing SDL_ttf here
        
        unsigned int CompileShader(unsigned int type, const std::string& source);
        unsigned int CreateShaderProgram(const std::string& vertexSrc, const std::string& fragmentSrc);
    };

} // namespace VECTOR
