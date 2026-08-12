#include "Engine/Graphics/Vulkan/VulkanRenderer.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/Vulkan/VulkanDescriptorManager.hpp"
#include "Engine/Graphics/Vulkan/VulkanTexture2D.hpp"
#include "Engine/Graphics/Vulkan/VulkanUniformBuffer.hpp"
#include "Engine/Graphics/Material.hpp"
#include <algorithm>

namespace VECTOR {

std::shared_ptr<Texture2D> VulkanRenderer::AllocateTransientTexture(uint32_t handle, uint32_t width, uint32_t height, TextureFormat format, Texture2D* aliasTexture) {
  if (aliasTexture) {
    auto vkTex = dynamic_cast<VulkanTexture2D*>(aliasTexture);
    if (vkTex && vkTex->GetAllocation() != VK_NULL_HANDLE) {
      return Texture2D::CreateRenderTargetAliased(width, height, format, vkTex);
    }
  }
  return Texture2D::CreateRenderTarget(width, height, format);
}

VkDescriptorSet VulkanRenderer::GetOrCreateObjectDescriptorSet(const RenderCommand &cmd) {
  if (cmd.boneTransforms && !cmd.boneTransforms->empty()) {
    if (m_ObjectDataIndex >= m_ObjectDataPool.size()) {
      ObjectData data;
      data.ubo = std::make_unique<VulkanUniformBuffer>(static_cast<uint32_t>(sizeof(glm::mat4) * 100), 0);
      data.descriptorSet = m_DescriptorManager->AllocateObjectSet();

      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = data.ubo->GetBuffer();
      bufferInfo.offset = 0;
      bufferInfo.range = sizeof(glm::mat4) * 100;

      VkWriteDescriptorSet descriptorWrite{};
      descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrite.dstSet = data.descriptorSet;
      descriptorWrite.dstBinding = 0;
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pBufferInfo = &bufferInfo;

      vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &descriptorWrite, 0, nullptr);

      m_ObjectDataPool.push_back(std::move(data));
    }

    m_ObjectDataPool[m_ObjectDataIndex].ubo->SetData(
        cmd.boneTransforms->data(),
        std::min((int)cmd.boneTransforms->size(), 100) * sizeof(glm::mat4));
    VkDescriptorSet set = m_ObjectDataPool[m_ObjectDataIndex].descriptorSet;
    m_ObjectDataIndex++;
    return set;
  }
  return m_DummyObjectSet;
}

VkDescriptorSet VulkanRenderer::GetOrCreateMaterialDescriptorSet(const Material *material) {
  if (!material)
    return m_DummyMaterialDescriptorSet;

  MaterialTextures matTex = {
      material->albedoTexture.get(), material->normalTexture.get(),
      material->metallicRoughnessTexture.get(), material->aoTexture.get()};

  auto it = m_MaterialDescriptorSets.find(matTex);
  if (it != m_MaterialDescriptorSets.end()) {
    return it->second;
  }

  VkDevice device = m_Context->GetDevice();
  VkDescriptorSet newSet = m_DescriptorManager->AllocateMaterialSet();

  if (newSet != VK_NULL_HANDLE) {
    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorImageInfo> imageInfos(4);

    const Texture2D *texArray[4] = {matTex.albedo, matTex.normal, matTex.mr,
                                    matTex.ao};

    for (uint32_t i = 0; i < 4; ++i) {
      auto vulkanTex = dynamic_cast<const VulkanTexture2D *>(texArray[i]);
      if (!vulkanTex)
        vulkanTex = m_DummyTexture.get();

      imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfos[i].imageView = vulkanTex->GetImageView();
      imageInfos[i].sampler = vulkanTex->GetSampler();

      VkWriteDescriptorSet descriptorWrite{};
      descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrite.dstSet = newSet;
      descriptorWrite.dstBinding = i;
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pImageInfo = &imageInfos[i];
      writes.push_back(descriptorWrite);
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()),
                           writes.data(), 0, nullptr);
    m_MaterialDescriptorSets[matTex] = newSet;
    return newSet;
  }

  VECTOR_LOG_ERROR("Failed to allocate descriptor set for material!");
  return m_DummyMaterialDescriptorSet;
}

} // namespace VECTOR
