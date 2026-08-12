#pragma once

#include "Engine/Graphics/Material.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Shader.hpp"
#include "Engine/Graphics/RenderGraph.hpp"
#include "Engine/Graphics/Texture2D.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>


namespace VECTOR {

class Renderer {
public:
  virtual ~Renderer() = default;

  virtual bool Initialize(const std::string &title, int width, int height) = 0;
  virtual void Shutdown() = 0;
  virtual void WaitIdle() {}

  virtual void Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) = 0;
  virtual void Present() = 0;

  virtual void SetResolution(int width, int height) = 0;
  virtual void SetFullscreen(bool fullscreen, bool borderless) = 0;

  virtual void SetViewProjection(const glm::vec3 &viewPos,
                                 const glm::mat4 &view,
                                 const glm::mat4 &projection) = 0;

  virtual void
  SubmitMesh(const Mesh *mesh, const Material *material, const glm::mat4 &model,
             const std::vector<glm::mat4> *boneTransforms = nullptr);

  virtual void SubmitPointLight(const glm::vec3 &position, float radius,
                                const glm::vec3 &color, float intensity) = 0;
  virtual void SetDirectionalLight(const glm::vec3 &direction,
                                   const glm::vec3 &color, float intensity) = 0;

  virtual void SubmitSkybox(class Cubemap *cubemap) {}

  virtual void BeginUI() = 0;
  virtual void DrawUIRect(int x, int y, int w, int h,
                          const glm::vec4 &color) = 0;
  virtual void DrawUIText(const std::string &text, int x, int y,
                          const glm::vec4 &color, int fontSize = 24) = 0;
  virtual void EndUI() = 0;

  virtual std::shared_ptr<Texture2D> AllocateTransientTexture(uint32_t handle, uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA16F, Texture2D* aliasTexture = nullptr) { return nullptr; }
  virtual void TransitionResources(const std::vector<RGResourceTransition>& transitions) {}

  virtual void BeginImGuiFrame() = 0;
  virtual void EndImGuiFrame() = 0;

  virtual SDL_Window *GetWindow() const = 0;

  // Multi-Pass Rendering Methods (Protected/Internal hooks for the backends)
  virtual void BeginShadowPass() = 0;
  virtual void FlushShadowPass() = 0;

  virtual void BeginPrepass(std::shared_ptr<Texture2D> outNormal, std::shared_ptr<Texture2D> outPosition, std::shared_ptr<Texture2D> outDepth) = 0;
  virtual void FlushPrepass() = 0;

  virtual void BeginMainPass(std::shared_ptr<Texture2D> inNormal, std::shared_ptr<Texture2D> inPosition, std::shared_ptr<Texture2D> inDepth, std::shared_ptr<Texture2D> outColor) = 0;
  virtual void FlushMainPass() = 0;
  virtual void EndPostProcessPass(std::shared_ptr<Texture2D> inColor) = 0;
  
  virtual void GenerateSSAO() {}

  virtual const glm::mat4 &GetLightSpaceMatrix() const = 0;
  virtual Shader *GetDepthShader() const = 0;
  virtual Material *GetDefaultMaterial() const = 0;
  virtual void SetUnlitMode(bool unlit) = 0;

  virtual void SetWireframeMode(bool enabled) = 0;
  virtual std::string GetRendererInfo() const = 0;

  virtual uint32_t GetDrawCallCount() const = 0;

  virtual void SetSSAOEnabled(bool enabled) {}
  virtual bool IsSSAOEnabled() const { return false; }

  virtual void SetBloomEnabled(bool enabled) {}
  virtual bool IsBloomEnabled() const { return false; }

  virtual void SetTAAEnabled(bool enabled) {}
  virtual bool IsTAAEnabled() const { return false; }

  static std::unique_ptr<Renderer> Create();

  // --- Render Pipeline ---
  // Executes the entire rendering loop
  virtual void ExecuteRenderPipeline();

protected:
  struct RenderCommand {
    const Mesh *mesh;
    const Material *material;
    glm::mat4 model;
    const std::vector<glm::mat4> *boneTransforms = nullptr;
  };

  std::vector<RenderCommand> m_RenderQueue;
  RenderGraph m_RenderGraph;
};

} // namespace VECTOR
