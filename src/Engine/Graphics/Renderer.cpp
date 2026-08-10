#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/RendererAPI.hpp"

#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Renderer.hpp"
#include "Engine/Graphics/Vulkan/VulkanRenderer.hpp"


namespace VECTOR {

std::unique_ptr<Renderer> Renderer::Create() {
  switch (RendererAPI::GetAPI()) {
  case RendererAPI::API::None:
    VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
    return nullptr;

  case RendererAPI::API::Vulkan:
    return std::make_unique<VulkanRenderer>();
  case RendererAPI::API::DirectX12:
    return std::make_unique<DirectX12Renderer>();
  }

  VECTOR_LOG_ERROR("Unknown RendererAPI!");
  return nullptr;
}

void Renderer::SubmitMesh(const Mesh *mesh, const Material *material,
                          const glm::mat4 &model,
                          const std::vector<glm::mat4> *boneTransforms) {
  RenderCommand cmd;
  cmd.mesh = mesh;
  cmd.material = material;
  cmd.model = model;
  cmd.boneTransforms = boneTransforms;
  m_RenderQueue.push_back(cmd);
}

void Renderer::ExecuteRenderPipeline() {
  // Execute the multi-pass rendering pipeline using the unified render queue

  BeginShadowPass();
  FlushShadowPass(); // Backends will loop through m_RenderQueue and draw shadow
                     // casters

  BeginPrepass();
  FlushPrepass(); // Backends will loop through m_RenderQueue and draw to
                  // G-Buffer

  if (IsSSAOEnabled()) {
    // Generate SSAO based on G-Buffer
    // The backend is responsible for intercepting this if necessary or SSAO is
    // already implicitly chained.
  }

  BeginMainPass();
  FlushMainPass(); // Backends will loop through m_RenderQueue and draw to HDR
                   // buffer

  EndPostProcessPass();

  m_RenderQueue.clear();
}

} // namespace VECTOR
