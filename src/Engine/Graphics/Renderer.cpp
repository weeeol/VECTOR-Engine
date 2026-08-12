#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/RenderGraph.hpp"

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
  if (m_RenderGraph.IsEmpty()) {
    RGResourceHandle shadowMap = RG_INVALID_HANDLE;
    RGResourceHandle gBufferNormal = RG_INVALID_HANDLE;
    RGResourceHandle gBufferPosition = RG_INVALID_HANDLE;
    RGResourceHandle gBufferDepth = RG_INVALID_HANDLE;
    RGResourceHandle hdrTarget = RG_INVALID_HANDLE;

    m_RenderGraph.AddPass("ShadowPass", 
      [&](RenderGraphBuilder& builder) {
        shadowMap = builder.CreateTexture("ShadowMap", {2048, 2048, TextureFormat::Depth32F});
      }, 
      std::make_unique<LambdaRenderPass>([this](Renderer* r) {
        r->BeginShadowPass();
        r->FlushShadowPass();
      })
    );

    m_RenderGraph.AddPass("Prepass", 
      [&](RenderGraphBuilder& builder) {
        builder.ReadTexture(shadowMap);
        gBufferNormal = builder.CreateTexture("GBufferNormal", {1920, 1080, TextureFormat::RGBA16F});
        gBufferPosition = builder.CreateTexture("GBufferPosition", {1920, 1080, RendererAPI::GetAPI() == RendererAPI::API::Vulkan ? TextureFormat::RGBA32F : TextureFormat::RG16F});
        gBufferDepth = builder.CreateTexture("GBufferDepth", {1920, 1080, TextureFormat::Depth32F});
      }, 
      std::make_unique<LambdaRenderPass>([&graph = m_RenderGraph, this](Renderer* r) {
        r->BeginPrepass(graph.GetTexture("GBufferNormal"), graph.GetTexture("GBufferPosition"), graph.GetTexture("GBufferDepth"));
        r->FlushPrepass();
      })
    );

    if (IsSSAOEnabled()) {
      m_RenderGraph.AddPass("SSAO",
        [&](RenderGraphBuilder& builder) {
          builder.ReadTexture(gBufferNormal);
          builder.ReadTexture(gBufferDepth);
        },
        std::make_unique<LambdaRenderPass>([this](Renderer* r) {
          r->GenerateSSAO();
        })
      );
    }

    m_RenderGraph.AddPass("MainPass", 
      [&](RenderGraphBuilder& builder) {
        builder.ReadTexture(gBufferNormal);
        builder.ReadTexture(gBufferPosition);
        builder.ReadTexture(gBufferDepth);
        hdrTarget = builder.CreateTexture("HDRTarget", {1920, 1080, TextureFormat::RGBA16F});
      }, 
      std::make_unique<LambdaRenderPass>([&graph = m_RenderGraph, this](Renderer* r) {
        r->BeginMainPass(graph.GetTexture("GBufferNormal"), graph.GetTexture("GBufferPosition"), graph.GetTexture("GBufferDepth"),
                         graph.GetTexture("HDRTarget"));
        r->FlushMainPass();
      })
    );

    m_RenderGraph.AddPass("PostProcess", 
      [&](RenderGraphBuilder& builder) {
        builder.ReadTexture(hdrTarget);
      }, 
      std::make_unique<LambdaRenderPass>([&graph = m_RenderGraph, this](Renderer* r) {
        r->EndPostProcessPass(graph.GetTexture("HDRTarget"));
      })
    );

    m_RenderGraph.Compile();
  }

  m_RenderGraph.Execute(this);

  m_RenderQueue.clear();
}

} // namespace VECTOR
