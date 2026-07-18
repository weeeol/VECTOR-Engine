#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace VECTOR {

    class VulkanRenderer : public Renderer {
    public:
        VulkanRenderer();
        virtual ~VulkanRenderer();

        virtual bool Initialize(const std::string& title, int width, int height) override;
        virtual void Shutdown() override;

        virtual void Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) override;
        virtual void Present() override;

        virtual void SetResolution(int width, int height) override;
        virtual void SetFullscreen(bool fullscreen, bool borderless) override;

        virtual void SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) override;

        virtual void SubmitMesh(const Mesh* mesh, const Material* material, const glm::mat4& model) override;

        virtual void SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) override;
        virtual void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) override;

        virtual void BeginUI() override;
        virtual void DrawUIRect(int x, int y, int w, int h, const glm::vec4& color) override;
        virtual void DrawUIText(const std::string& text, int x, int y, const glm::vec4& color, int fontSize = 24) override;
        virtual void EndUI() override;

        virtual void BeginImGuiFrame() override;
        virtual void EndImGuiFrame() override;

        virtual SDL_Window* GetWindow() const override { return m_Window; }

        virtual void BeginShadowPass() override;
        virtual void FlushShadowPass() override;
        virtual void BeginMainPass() override;
        virtual void FlushMainPass() override;
        virtual void EndPostProcessPass() override;

        virtual const glm::mat4& GetLightSpaceMatrix() const override { return m_DummyMatrix; }
        virtual Shader* GetDepthShader() const override { return nullptr; }
        virtual Material* GetDefaultMaterial() const override { return nullptr; }
        virtual void SetUnlitMode(bool unlit) override {}
        
        virtual void SetWireframeMode(bool enabled) override {}
        virtual std::string GetRendererInfo() const override { return "Vulkan Renderer (Stub)"; }

        virtual uint32_t GetDrawCallCount() const override { return 0; }

    private:
        SDL_Window* m_Window = nullptr;
        glm::mat4 m_DummyMatrix = glm::mat4(1.0f);
    };

} // namespace VECTOR
