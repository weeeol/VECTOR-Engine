#pragma once

#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include <unordered_map>

#include <GL/glew.h>
#include <SDL_opengl.h>
#include <glm/glm.hpp>
#include <memory>
#include "Engine/Graphics/Shader.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include "Engine/Graphics/Material.hpp"
#include "Engine/Graphics/RenderQueue.hpp"
#include "Engine/Graphics/UniformBufferObject.hpp"
#include "Engine/Graphics/Frustum.hpp"

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

        /**
         * @brief Submit a mesh draw call to the appropriate render queue.
         * Performs frustum culling. This does NOT immediately issue GL calls.
         * @param mesh The mesh geometry to draw.
         * @param material The material (shader + texture + properties). If null, uses default material.
         * @param model The model transformation matrix.
         */
        void SubmitMesh(const Mesh* mesh, const Material* material, const glm::mat4& model);

        void SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity);
        void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity);


        void BeginUI();
        void DrawUIRect(int x, int y, int w, int h, const glm::vec4& color);
        void DrawUIText(const std::string& text, int x, int y, const glm::vec4& color, int fontSize = 24);
        void EndUI();


        SDL_Window* GetWindow() const { return m_Window; }

        // Multi-Pass Rendering Methods
        void BeginShadowPass();
        void FlushShadowPass();
        void BeginMainPass();
        void FlushMainPass();
        void EndPostProcessPass();

        const glm::mat4& GetLightSpaceMatrix() const { return m_LightSpaceMatrix; }
        Shader* GetDepthShader() const { return m_DepthShader.get(); }
        Material* GetDefaultMaterial() const { return m_DefaultMaterial.get(); }
        void SetUnlitMode(bool unlit) { m_UnlitMode = unlit; }

        /** @brief Get the number of draw calls from the last rendered frame. */
        uint32_t GetDrawCallCount() const { return m_LastFrameDrawCalls; }

    private:
        SDL_Window* m_Window;
        SDL_GLContext m_GLContext;
        std::shared_ptr<Shader> m_DefaultShader;
        std::shared_ptr<Shader> m_DepthShader;
        std::shared_ptr<Shader> m_PostProcessShader;
        
        bool m_UnlitMode = false;

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

        unsigned int m_ScreenQuadVAO, m_ScreenQuadVBO;
        
        void SetupFBOs();
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

        // =====================================================================
        // Bloom Chain
        // =====================================================================
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

