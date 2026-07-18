#include "Engine/Graphics/Vulkan/VulkanShader.hpp"
#include "Engine/Graphics/Vulkan/VulkanContext.hpp"
#include "Engine/Core/Logger.hpp"
#include <fstream>
#include <iostream>

namespace VECTOR {

    VulkanShader::VulkanShader(const std::string& vertexPath, const std::string& fragmentPath) {
        // Assume the paths might not have .spv, we should append it or load the spv path directly.
        // But since we put our vulkan shaders in assets/engine/shaders/vulkan, let's load from there.
        // For simplicity, we just load vertexPath + ".spv" if it's not a pre-compiled path,
        // Actually, the compile_shaders.bat creates .spv in the same folder.
        std::string vPath = vertexPath;
        std::string fPath = fragmentPath;
        
        // If it doesn't end with .spv, let's just append it, and replace "shaders/" with "shaders/vulkan/"
        // since our vulkan shaders are in the vulkan subfolder.
        if (vPath.find(".spv") == std::string::npos) {
            size_t pos = vPath.find("shaders/");
            if (pos != std::string::npos) {
                vPath.replace(pos, 8, "shaders/vulkan/");
            }
            vPath += ".spv";
        }
        
        if (fPath.find(".spv") == std::string::npos) {
            size_t pos = fPath.find("shaders/");
            if (pos != std::string::npos) {
                fPath.replace(pos, 8, "shaders/vulkan/");
            }
            fPath += ".spv";
        }

        auto vertCode = ReadFile(vPath);
        auto fragCode = ReadFile(fPath);

        m_VertexModule = CreateShaderModule(vertCode);
        m_FragmentModule = CreateShaderModule(fragCode);
        
        if (m_VertexModule != VK_NULL_HANDLE && m_FragmentModule != VK_NULL_HANDLE) {
            VECTOR_LOG_INFO("Loaded Vulkan shader: " + vPath + " & " + fPath);
        } else {
            VECTOR_LOG_ERROR("Failed to load Vulkan shader modules!");
        }
    }

    VulkanShader::~VulkanShader() {
        VkDevice device = VulkanContext::Get()->GetDevice();
        if (m_VertexModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, m_VertexModule, nullptr);
        }
        if (m_FragmentModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, m_FragmentModule, nullptr);
        }
    }

    std::vector<char> VulkanShader::ReadFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            VECTOR_LOG_ERROR("Failed to open Vulkan shader file: " + filename);
            return std::vector<char>();
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    VkShaderModule VulkanShader::CreateShaderModule(const std::vector<char>& code) {
        if (code.empty()) return VK_NULL_HANDLE;

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(VulkanContext::Get()->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            VECTOR_LOG_ERROR("Failed to create shader module!");
            return VK_NULL_HANDLE;
        }

        return shaderModule;
    }

    void VulkanShader::Bind() const {
        // Vulkan binds pipelines, not individual shaders.
        // This function is kept for API compatibility but might be a no-op 
        // or used to set state in a command buffer.
    }

    void VulkanShader::Unbind() const {
    }

    void VulkanShader::SetInt(const std::string& name, int value) const {
    }

    void VulkanShader::SetFloat(const std::string& name, float value) const {
    }

    void VulkanShader::SetVec2(const std::string& name, const glm::vec2& value) const {
    }

    void VulkanShader::SetVec3(const std::string& name, const glm::vec3& value) const {
    }

    void VulkanShader::SetVec4(const std::string& name, const glm::vec4& value) const {
    }

    void VulkanShader::SetMat4(const std::string& name, const glm::mat4& value) const {
    }

} // namespace VECTOR
