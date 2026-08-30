#include "Engine/Graphics/Vulkan/VulkanRenderer.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/Vulkan/VulkanCubemap.hpp"
#include "Engine/Graphics/Vulkan/VulkanDescriptorManager.hpp"
#include "Engine/Graphics/Vulkan/VulkanMesh.hpp"
#include "Engine/Graphics/Vulkan/VulkanPostProcessor.hpp"
#include "Engine/Graphics/Vulkan/VulkanPrepass.hpp"
#include "Engine/Graphics/Vulkan/VulkanSSAO.hpp"
#include "Engine/Graphics/Vulkan/VulkanShadowPass.hpp"
#include "Engine/Graphics/Vulkan/VulkanTexture2D.hpp"
#include "Engine/Graphics/Vulkan/VulkanUniformBuffer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL_vulkan.h>

namespace {
float Halton(int index, int base) {
  float f = 1.0f;
  float r = 0.0f;
  int current = index;
  while (current > 0) {
    f = f / base;
    r = r + f * (current % base);
    current = current / base;
  }
  return r;
}
} // namespace
namespace VECTOR {

void VulkanRenderer::SetViewProjection(const glm::vec3 &viewPos,
                                       const glm::mat4 &view,
                                       const glm::mat4 &projection) {
  if (!m_FrameStarted)
    BeginFrame();

  m_PrevView = m_CachedView;
  m_PrevProjection = m_CachedProjection;

  m_CachedView = view;
  m_CachedProjection = projection;

  glm::mat4 jitteredProjection = projection;

  if (m_TAAEnabled) {
    m_TAAJitterIndex = (m_TAAJitterIndex + 1) % 16;
    float jitterX = (Halton(m_TAAJitterIndex + 1, 2) - 0.5f);
    float jitterY = (Halton(m_TAAJitterIndex + 1, 3) - 0.5f);

    int width, height;
    SDL_GetWindowSizeInPixels(m_Window, &width, &height);

    m_TAAJitter = glm::vec2(jitterX * 2.0f / width, jitterY * 2.0f / height);

    jitteredProjection[2][0] += m_TAAJitter.x;
    jitteredProjection[2][1] += m_TAAJitter.y;
  } else {
    m_TAAJitter = glm::vec2(0.0f);
  }

  PerFrameData data{};
  data.view = view;
  data.projection = jitteredProjection;
  data.skyboxIndex = -1; // Unused in Vulkan currently
  // The Y axis is flipped in Vulkan clip space compared to OpenGL
  data.projection[1][1] *= -1.0f;

  data.prevView = m_PrevView;
  data.prevProjection = m_PrevProjection;
  // If m_PrevProjection wasn't flipped, we flip it here:
  data.prevProjection[1][1] *= -1.0f;

  data.jitter = m_TAAJitter;

  glm::vec3 sunDir = glm::normalize(glm::vec3(-0.2f, 1.0f, 0.3f));
  glm::vec3 lightPos = sunDir * 80.0f;
  glm::mat4 lightProjection =
      glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 1.0f, 150.0f);
  // Flip Y for Vulkan projection
  lightProjection[1][1] *= -1.0f;
  glm::mat4 lightView =
      glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
  m_LightSpaceMatrix = lightProjection * lightView;

  data.lightSpaceMatrix = m_LightSpaceMatrix;
  data.viewPos = glm::vec4(viewPos, 1.0f);

  // Fill some default directional light data so scene isn't completely black
  data.sunDir = glm::vec4(sunDir, 0.0f);
  data.sunColor = glm::vec4(1.0f);
  data.ssaoEnabled = m_SSAOEnabled ? 1 : 0;

  m_PerFrameUBOs[m_CurrentFrame]->SetData(&data, sizeof(PerFrameData), 0);
}

void VulkanRenderer::SubmitPointLight(const glm::vec3 &position, float radius,
                                      const glm::vec3 &color, float intensity) {
  if (m_LightData.numPointLights < MAX_POINT_LIGHTS) {
    int idx = m_LightData.numPointLights++;
    m_LightData.pointLights[idx].positionAndRadius =
        glm::vec4(position, radius);
    m_LightData.pointLights[idx].colorAndIntensity =
        glm::vec4(color, intensity);
  }
}

void VulkanRenderer::SetDirectionalLight(const glm::vec3 &direction,
                                         const glm::vec3 &color,
                                         float intensity) {
  m_LightData.dirLight.direction = glm::vec4(glm::normalize(direction), 0.0f);
  m_LightData.dirLight.colorAndIntensity = glm::vec4(color, intensity);
}

void VulkanRenderer::SubmitSkybox(Cubemap *cubemap) {
  m_CurrentSkybox = dynamic_cast<VulkanCubemap *>(cubemap);
}

VkImageLayout MapRGStateToVkLayout(int state) {
  switch (static_cast<RGResourceState>(state)) {
    case RGResourceState::RENDER_TARGET: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RGResourceState::DEPTH_WRITE: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case RGResourceState::DEPTH_READ: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case RGResourceState::SHADER_RESOURCE: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case RGResourceState::UNORDERED_ACCESS: return VK_IMAGE_LAYOUT_GENERAL;
    case RGResourceState::PRESENT: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default: return VK_IMAGE_LAYOUT_UNDEFINED;
  }
}

void VulkanRenderer::TransitionResources(const std::vector<RGResourceTransition>& transitions) {
  if (!m_FrameStarted || transitions.empty()) return;

  VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];

  std::vector<VkImageMemoryBarrier> barriers;
  barriers.reserve(transitions.size());

  VkPipelineStageFlags sourceStage = 0;
  VkPipelineStageFlags destinationStage = 0;

  for (const auto& trans : transitions) {
    auto tex = m_RenderGraph.GetTexture(trans.handle);
    if (!tex) continue;

    auto vkTex = dynamic_cast<VulkanTexture2D*>(tex.get());
    if (!vkTex || vkTex->GetImage() == VK_NULL_HANDLE) continue;

    VkImageLayout oldLayout = MapRGStateToVkLayout(trans.oldState);
    VkImageLayout newLayout = MapRGStateToVkLayout(trans.newState);

    if (oldLayout == newLayout) continue;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vkTex->GetImage();
    bool isDepth = (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) || 
                   (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) ||
                   (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) ||
                   (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) ||
                   (oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ||
                   (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ||
                   (oldLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) ||
                   (newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
    barrier.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    switch (oldLayout) {
      case VK_IMAGE_LAYOUT_UNDEFINED:
        barrier.srcAccessMask = 0;
        srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        break;
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        break;
      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStageFlags = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        break;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
      default:
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        srcStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        break;
    }

    switch (newLayout) {
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        break;
      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        break;
      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
      case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        barrier.dstAccessMask = 0;
        dstStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        break;
      default:
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        break;
    }

    sourceStage |= srcStageFlags;
    destinationStage |= dstStageFlags;

    barriers.push_back(barrier);
  }

  if (!barriers.empty()) {
    vkCmdPipelineBarrier(
      commandBuffer,
      sourceStage, destinationStage,
      0,
      0, nullptr,
      0, nullptr,
      static_cast<uint32_t>(barriers.size()), barriers.data()
    );
  }
}

void VulkanRenderer::BeginShadowPass() {
  BeginFrame();
  if (!m_FrameStarted)
    return;

  VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
  m_ShadowPass->BeginPass(commandBuffer);
}

void VulkanRenderer::FlushShadowPass() {
  if (!m_FrameStarted)
    return;
  VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];

  m_ShadowPass->GetDepthPipeline()->Bind(commandBuffer);

  for (const auto &cmd : m_RenderQueue) {
    VkDescriptorSet objSet = GetOrCreateObjectDescriptorSet(cmd);
    VkDescriptorSet sets[] = {m_GlobalDescriptorSets[m_CurrentFrame],
                              m_DummyMaterialDescriptorSet, objSet};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_DescriptorManager->GetPipelineLayout(), 0, 3,
                            sets, 0, nullptr);

    struct PushConstants {
      glm::mat4 model;
    } pc;
    pc.model = cmd.model;

    vkCmdPushConstants(commandBuffer, m_DescriptorManager->GetPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(glm::mat4), &pc);

    if (cmd.mesh) {
      auto vulkanMesh = dynamic_cast<const VulkanMesh *>(cmd.mesh);
      if (vulkanMesh) {
        VkBuffer vertexBuffers[] = {vulkanMesh->GetVertexBuffer()->GetBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer,
                             vulkanMesh->GetIndexBuffer()->GetBuffer(), 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, vulkanMesh->GetIndexCount(), 1, 0, 0,
                         0);
        m_DrawCallCount++;
      }
    }
  }

  m_ShadowPass->EndPass(commandBuffer);
}

void VulkanRenderer::BeginPrepass(std::shared_ptr<Texture2D> outNormal, std::shared_ptr<Texture2D> outPosition, std::shared_ptr<Texture2D> outDepth) {
  BeginFrame();
  if (!m_FrameStarted)
    return;

  VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
  m_Prepass->BeginPass(commandBuffer, outNormal, outPosition, outDepth);
}

void VulkanRenderer::FlushPrepass() {
  if (!m_FrameStarted)
    return;
  VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];

  m_Prepass->GetPipeline()->Bind(commandBuffer);

  for (const auto &cmd : m_RenderQueue) {
    VkDescriptorSet objSet = GetOrCreateObjectDescriptorSet(cmd);
    VkDescriptorSet sets[] = {m_GlobalDescriptorSets[m_CurrentFrame],
                              m_DummyMaterialDescriptorSet, objSet};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_DescriptorManager->GetPipelineLayout(), 0, 3,
                            sets, 0, nullptr);

    struct PushConstants {
      glm::mat4 model;
    } pc;
    pc.model = cmd.model;

    vkCmdPushConstants(commandBuffer, m_DescriptorManager->GetPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(glm::mat4), &pc);

    if (cmd.mesh) {
      auto vulkanMesh = dynamic_cast<const VulkanMesh *>(cmd.mesh);
      if (vulkanMesh) {
        VkBuffer vertexBuffers[] = {vulkanMesh->GetVertexBuffer()->GetBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer,
                             vulkanMesh->GetIndexBuffer()->GetBuffer(), 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, vulkanMesh->GetIndexCount(), 1, 0, 0,
                         0);
        // Don't increment draw call count here to avoid double counting
      }
    }
  }

  m_Prepass->EndPass(commandBuffer);
}

void VulkanRenderer::GenerateSSAO() {
  if (m_SSAOEnabled && m_FrameStarted) {
    VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
    m_SSAO->Generate(commandBuffer, m_Prepass->GetNormalImageView(),
                     m_Prepass->GetDepthImageView(), m_CachedProjection,
                     m_CachedView);
  }
}

void VulkanRenderer::BeginMainPass(std::shared_ptr<Texture2D> inNormal, std::shared_ptr<Texture2D> inPosition, std::shared_ptr<Texture2D> inDepth, std::shared_ptr<Texture2D> outColor) {
  if (m_MainPassActive)
    return;

  BeginFrame();
  if (!m_FrameStarted)
    return;

  auto vkColor = std::dynamic_pointer_cast<VulkanTexture2D>(outColor);
  auto vkDepth = std::dynamic_pointer_cast<VulkanTexture2D>(inDepth);

  if (!vkColor || !vkDepth) {
      VECTOR_LOG_ERROR("VulkanRenderer received invalid textures for Main Pass!");
      return;
  }

  if (m_MainFramebuffer == VK_NULL_HANDLE) {
      std::array<VkImageView, 3> fbAttachments = {
          vkColor->GetImageView(),
          m_PostProcessor->GetOffscreenVelocityView(),
          vkDepth->GetImageView()
      };

      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = m_PostProcessor->GetOffscreenRenderPass();
      framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
      framebufferInfo.pAttachments = fbAttachments.data();
      framebufferInfo.width = m_Swapchain->GetExtent().width;
      framebufferInfo.height = m_Swapchain->GetExtent().height;
      framebufferInfo.layers = 1;

      if (vkCreateFramebuffer(m_Context->GetDevice(), &framebufferInfo, nullptr, &m_MainFramebuffer) != VK_SUCCESS) {
          VECTOR_LOG_ERROR("Failed to create dynamic main framebuffer!");
          return;
      }
  }

  VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_PostProcessor->GetOffscreenRenderPass();
  renderPassInfo.framebuffer = m_MainFramebuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = m_Swapchain->GetExtent();

  std::vector<VkClearValue> clearValues(3);
  clearValues[0].color = {
      {m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a}};
  clearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
  clearValues[2].depthStencil = {1.0f, 0};

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  m_MainPassActive = true;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(m_Swapchain->GetExtent().width);
  viewport.height = static_cast<float>(m_Swapchain->GetExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = m_Swapchain->GetExtent();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulkanRenderer::FlushMainPass() {
  if (!m_FrameStarted)
    return;

  // Update Light UBO before drawing
  m_LightUBOs[m_CurrentFrame]->SetData(&m_LightData, sizeof(LightUBOData), 0);
  m_LightData.numPointLights = 0;

  VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];

  if (!m_RenderQueue.empty()) {
    // Bind Pipeline
    if (m_IsWireframe) {
      m_WireframePipeline->Bind(commandBuffer);
    } else {
      m_Pipeline->Bind(commandBuffer);
    }

  for (const auto &cmd : m_RenderQueue) {
    VkDescriptorSet objSet = GetOrCreateObjectDescriptorSet(cmd);
    VkDescriptorSet materialSet =
        cmd.material ? GetOrCreateMaterialDescriptorSet(cmd.material)
                     : m_DummyMaterialDescriptorSet;
    VkDescriptorSet sets[] = {m_GlobalDescriptorSets[m_CurrentFrame],
                              materialSet, objSet};
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_DescriptorManager->GetPipelineLayout(), 0, 3,
                            sets, 0, nullptr);
    // Material Push Constants
    struct PushConstants {
      glm::mat4 model;
      glm::vec4 color;
      uint32_t hasAlbedoMap;
      uint32_t hasNormalMap;
      uint32_t hasMetallicRoughnessMap;
      uint32_t hasAOMap;
      float metallicFactor;
      float roughnessFactor;
      uint32_t isUnlit;
      uint32_t _padding;
    } pc;

    pc.model = cmd.model;

    if (cmd.material) {
      pc.color = cmd.material->albedoColor;
      pc.hasAlbedoMap = cmd.material->albedoTexture ? 1 : 0;
      pc.hasNormalMap = cmd.material->normalTexture ? 1 : 0;
      pc.hasMetallicRoughnessMap =
          cmd.material->metallicRoughnessTexture ? 1 : 0;
      pc.hasAOMap = cmd.material->aoTexture ? 1 : 0;
      pc.metallicFactor = cmd.material->metallic;
      pc.roughnessFactor = cmd.material->roughness;
      pc.isUnlit = cmd.material->isUnlit ? 1 : 0;
    } else {
      pc.color = glm::vec4(1.0f);
      pc.hasAlbedoMap = 0;
      pc.hasNormalMap = 0;
      pc.hasMetallicRoughnessMap = 0;
      pc.hasAOMap = 0;
      pc.metallicFactor = 0.0f;
      pc.roughnessFactor = 0.5f;
      pc.isUnlit = 0;
    }

    vkCmdPushConstants(commandBuffer, m_DescriptorManager->GetPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(PushConstants), &pc);

    // Bind Mesh Buffers and Draw
    auto vulkanMesh = dynamic_cast<const VulkanMesh *>(cmd.mesh);
    if (vulkanMesh) {
      VkBuffer vertexBuffers[] = {vulkanMesh->GetVertexBuffer()->GetBuffer()};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

      vkCmdBindIndexBuffer(commandBuffer,
                           vulkanMesh->GetIndexBuffer()->GetBuffer(), 0,
                           VK_INDEX_TYPE_UINT32);

      vkCmdDrawIndexed(commandBuffer, vulkanMesh->GetIndexCount(), 1, 0, 0, 0);
      m_DrawCallCount++;
    }
  }
  } // End if (!m_RenderQueue.empty())

  if (m_CurrentSkybox && m_SkyboxPipeline) {
    m_SkyboxPipeline->Bind(commandBuffer);

    // Allocate a descriptor set for the skybox if not done or use a cached one.
    // For simplicity, we can reuse the dummy material set and update it or
    // allocate a new one. But allocating every frame isn't great. Let's just
    // create a new one every frame for now, or use m_SkyboxDescriptorSet.
    if (m_SkyboxDescriptorSet == VK_NULL_HANDLE) {
      m_SkyboxDescriptorSet = m_DescriptorManager->AllocateMaterialSet();
      if (m_SkyboxDescriptorSet != VK_NULL_HANDLE) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_CurrentSkybox->GetImageView();
        imageInfo.sampler = m_CurrentSkybox->GetSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_SkyboxDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &descriptorWrite, 0,
                               nullptr);
      }
    }

    if (m_SkyboxDescriptorSet != VK_NULL_HANDLE) {
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              m_DescriptorManager->GetPipelineLayout(), 1, 1,
                              &m_SkyboxDescriptorSet, 0, nullptr);
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              m_DescriptorManager->GetPipelineLayout(), 0, 1,
                              &m_GlobalDescriptorSets[m_CurrentFrame], 0,
                              nullptr);

      // Draw 36 vertices without vertex buffer
      vkCmdDraw(commandBuffer, 36, 1, 0, 0);
    }
  }

  m_RenderQueue.clear();
  m_CurrentSkybox = nullptr;

  if (m_MainPassActive) {
    vkCmdEndRenderPass(commandBuffer);
    m_MainPassActive = false;
  }
}

void VulkanRenderer::EndPostProcessPass(std::shared_ptr<Texture2D> inColor) {
  if (!m_FrameStarted) return;
  VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];

  m_LastMainColor = inColor;
  m_PostProcessor->Process(commandBuffer, inColor);
}

} // namespace VECTOR
