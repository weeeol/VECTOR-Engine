#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

#include "Engine/Core/Window.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/Shader.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include "Engine/Graphics/Buffer.hpp"
#include "Engine/Graphics/VertexArray.hpp"
#include "Engine/Graphics/Framebuffer.hpp"

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

        void SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection);
        void DrawMesh(const class Mesh* mesh, const class Shader* shader, const class Texture2D* texture, const glm::mat4& model, const glm::vec4& color);
        
        void BeginUI();
        void DrawUIRect(int x, int y, int w, int h, const glm::vec4& color);
        void DrawUIText(const std::string& text, int x, int y, const glm::vec4& color, int fontSize = 24);
        void EndUI();

        // Legacy 2D API (Will be removed later)
        void DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        void DrawText(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b, int fontSize = 24);

        Window* GetWindow() const { return m_Window.get(); }

        // New Multi-Pass Rendering Methods
        void BeginShadowPass();
        void BeginMainPass();
        void EndPostProcessPass();

        const glm::mat4& GetLightSpaceMatrix() const { return m_LightSpaceMatrix; }
        class Shader* GetDepthShader() const { return m_DepthShader.get(); }
        void SetUnlitMode(bool unlit) { m_UnlitMode = unlit; }

    private:
        std::unique_ptr<Window> m_Window;
        std::unique_ptr<RendererAPI> m_RendererAPI;

        std::shared_ptr<Shader> m_DefaultShader;
        std::shared_ptr<Shader> m_DepthShader;
        std::shared_ptr<Shader> m_PostProcessShader;
        
        bool m_UnlitMode = false;

        glm::vec3 m_ViewPos;
        glm::mat4 m_ViewMatrix;
        glm::mat4 m_ProjectionMatrix;
        glm::mat4 m_LightSpaceMatrix;
        // Framebuffers
        std::shared_ptr<Framebuffer> m_ShadowMapFramebuffer;
        const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;

        std::shared_ptr<Framebuffer> m_PostProcessFramebuffer;
        Framebuffer* m_ActiveFBO = nullptr;

        std::shared_ptr<VertexArray> m_ScreenQuadVertexArray;
        
        void SetupFBOs();
        void SetupScreenQuad();

        // 2D Rendering
        std::shared_ptr<Shader> m_2DShader;
        std::shared_ptr<VertexArray> m_QuadVertexArray;
        int m_Width, m_Height;
        struct TextTexture {
            unsigned int id; // Abstract later
            int w, h;
        };
        std::unordered_map<std::string, TextTexture> m_TextTextureCache;
    };

} // namespace VECTOR

