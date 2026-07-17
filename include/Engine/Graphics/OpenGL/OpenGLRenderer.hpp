#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>
#include "Engine/Graphics/RenderQueue.hpp"
#include "Engine/Graphics/UniformBufferObject.hpp"
#include "Engine/Graphics/Frustum.hpp"

namespace VECTOR {

    class OpenGLRenderer : public Renderer {
    public:
        OpenGLRenderer();
        ~OpenGLRenderer() override;

        OpenGLRenderer(const OpenGLRenderer&) = delete;
        OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;

        bool Initialize(const std::string& title, int width, int height) override;
        void Shutdown() override;

        void Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) override;
        void Present() override;

        void SetResolution(int width, int height) override;
        void SetFullscreen(bool fullscreen, bool borderless) override;

        void SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) override;

        void SubmitMesh(const Mesh* mesh, const Material* material, const glm::mat4& model) override;

        void SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) override;
        void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) override;

        void BeginUI() override;
        void DrawUIRect(int x, int y, int w, int h, const glm::vec4& color) override;
        void DrawUIText(const std::string& text, int x, int y, const glm::vec4& color, int fontSize = 24) override;
        void EndUI() override;

        void BeginImGuiFrame() override;
        void EndImGuiFrame() override;

        SDL_Window* GetWindow() const override { return m_Window; }

        void BeginShadowPass() override;
        void FlushShadowPass() override;
        void BeginMainPass() override;
        void FlushMainPass() override;
        void EndPostProcessPass() override;

        const glm::mat4& GetLightSpaceMatrix() const override { return m_LightSpaceMatrix; }
        Shader* GetDepthShader() const override { return m_DepthShader.get(); }
        Material* GetDefaultMaterial() const override { return m_DefaultMaterial.get(); }
        void SetUnlitMode(bool unlit) override { m_UnlitMode = unlit; }

        void SetWireframeMode(bool enabled) override;
        std::string GetRendererInfo() const override;

        uint32_t GetDrawCallCount() const override { return m_LastFrameDrawCalls; }

    private:
        SDL_Window* m_Window;
        SDL_GLContext m_GLContext;
        std::shared_ptr<Shader> m_DefaultShader;
        std::shared_ptr<Shader> m_DepthShader;
        std::shared_ptr<Shader> m_PostProcessShader;
        
        bool m_UnlitMode = false;
        bool m_WireframeEnabled = false;

        glm::vec3 m_ViewPos;
        glm::mat4 m_ViewMatrix;
        glm::mat4 m_ProjectionMatrix;
        glm::mat4 m_LightSpaceMatrix;
        
        // FBOs
        unsigned int m_DepthMapFBO;
        unsigned int m_DepthMapTexture;
        const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;

        unsigned int m_PostProcessFBO;
        unsigned int m_PostProcessTexture;
        unsigned int m_PostProcessRBO;

        // G-Buffer FBO & Textures
        unsigned int m_GBufferFBO = 0;
        unsigned int m_GPosition = 0;
        unsigned int m_GNormal = 0;
        unsigned int m_GAlbedo = 0;
        unsigned int m_GMetallicRoughnessAO = 0;
        unsigned int m_GBufferRBO = 0;
        std::shared_ptr<Shader> m_GBufferShader;
        std::shared_ptr<Shader> m_DeferredLightShader;

        unsigned int m_ScreenQuadVAO, m_ScreenQuadVBO;
        
        void SetupDepthFBO();
        void SetupPostProcessFBO();
        void SetupGBufferFBO();
        void RecreateScreenFBOs();
        void SetupScreenQuad();

        // Render Queues
        RenderQueue m_MainQueue;
        RenderQueue m_ShadowQueue;
        uint32_t m_LastFrameDrawCalls = 0;

        // Per-frame UBO
        std::unique_ptr<UniformBuffer> m_PerFrameUBO;
        PerFrameData m_PerFrameData;

        // Light UBO
        std::unique_ptr<UniformBuffer> m_LightUBO;
        LightUBOData m_LightData;

        // Frustum for culling
        Frustum m_CameraFrustum;

        // Default Material (used when none specified)
        std::shared_ptr<Material> m_DefaultMaterial;

        // Depth-only material for shadow pass
        std::shared_ptr<Material> m_DepthMaterial;

        // Bloom Chain
        static const int BLOOM_MIP_COUNT = 5;

        std::shared_ptr<Shader> m_BloomDownsampleShader;
        std::shared_ptr<Shader> m_BloomUpsampleShader;

        struct BloomMip {
            unsigned int fbo = 0;
            unsigned int texture = 0;
            int width = 0;
            int height = 0;
        };
        BloomMip m_BloomMips[BLOOM_MIP_COUNT];

        float m_BloomThreshold = 1.0f;
        float m_BloomStrength = 0.5f;
        float m_Exposure = 1.2f;
        float m_BloomFilterRadius = 0.005f;

        void SetupBloomFBOs();
        void RenderBloomPasses();

        int m_Width, m_Height;
        std::shared_ptr<Shader> m_UIShader;
        unsigned int m_UIQuadVAO, m_UIQuadVBO;
        
        struct TextTexture {
            unsigned int id;
            int w, h;
        };
        std::unordered_map<std::string, TextTexture> m_TextTextureCache;
    };

} // namespace VECTOR
