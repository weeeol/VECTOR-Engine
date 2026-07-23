#pragma once

#include <string>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <unordered_map>
#include <glm/glm.hpp>
#include <memory>
#include "Engine/Graphics/Shader.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include "Engine/Graphics/Material.hpp"

namespace VECTOR {

    class Renderer {
    public:
        virtual ~Renderer() = default;

        virtual bool Initialize(const std::string& title, int width, int height) = 0;
        virtual void Shutdown() = 0;
        virtual void WaitIdle() {}

        virtual void Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) = 0;
        virtual void Present() = 0;

        virtual void SetResolution(int width, int height) = 0;
        virtual void SetFullscreen(bool fullscreen, bool borderless) = 0;

        virtual void SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) = 0;

        virtual void SubmitMesh(const Mesh* mesh, const Material* material, const glm::mat4& model, const std::vector<glm::mat4>* boneTransforms = nullptr) = 0;

        virtual void SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) = 0;
        virtual void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) = 0;

        virtual void SubmitSkybox(class VulkanCubemap* cubemap) {}

        virtual void BeginUI() = 0;
        virtual void DrawUIRect(int x, int y, int w, int h, const glm::vec4& color) = 0;
        virtual void DrawUIText(const std::string& text, int x, int y, const glm::vec4& color, int fontSize = 24) = 0;
        virtual void EndUI() = 0;

        virtual void BeginImGuiFrame() = 0;
        virtual void EndImGuiFrame() = 0;

        virtual SDL_Window* GetWindow() const = 0;

        // Multi-Pass Rendering Methods
        virtual void BeginShadowPass() = 0;
        virtual void FlushShadowPass() = 0;
        virtual void BeginMainPass() = 0;
        virtual void FlushMainPass() = 0;
        virtual void EndPostProcessPass() = 0;

        virtual const glm::mat4& GetLightSpaceMatrix() const = 0;
        virtual Shader* GetDepthShader() const = 0;
        virtual Material* GetDefaultMaterial() const = 0;
        virtual void SetUnlitMode(bool unlit) = 0;
        
        virtual void SetWireframeMode(bool enabled) = 0;
        virtual std::string GetRendererInfo() const = 0;

        virtual uint32_t GetDrawCallCount() const = 0;

        static std::unique_ptr<Renderer> Create();
    };

} // namespace VECTOR
