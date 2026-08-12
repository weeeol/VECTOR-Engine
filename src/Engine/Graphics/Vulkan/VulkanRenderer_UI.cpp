#include "Engine/Graphics/Vulkan/VulkanRenderer.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

namespace VECTOR {

void VulkanRenderer::BeginUI() {
  // UI rendering handled by ImGui
}

void VulkanRenderer::DrawUIRect(int x, int y, int w, int h,
                                const glm::vec4 &color) {
  ImGui::GetBackgroundDrawList()->AddRectFilled(
      ImVec2((float)x, (float)y), ImVec2((float)(x + w), (float)(y + h)),
      ImColor(color.r, color.g, color.b, color.a));
}

void VulkanRenderer::DrawUIText(const std::string &text, int x, int y,
                                const glm::vec4 &color, int fontSize) {
  ImGui::GetBackgroundDrawList()->AddText(
      m_GameFont ? m_GameFont : ImGui::GetFont(), (float)fontSize,
      ImVec2((float)x, (float)y), ImColor(color.r, color.g, color.b, color.a),
      text.c_str());
}

void VulkanRenderer::EndUI() {
  // UI rendering handled by ImGui
}

void VulkanRenderer::BeginImGuiFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void VulkanRenderer::EndImGuiFrame() { ImGui::Render(); }

} // namespace VECTOR
