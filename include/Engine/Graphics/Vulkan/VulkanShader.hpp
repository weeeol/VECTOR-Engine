#pragma once

#include "Engine/Graphics/Shader.hpp"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>

namespace VECTOR {

    class VulkanShader : public Shader {
    public:
        VulkanShader(const std::string& vertexPath, const std::string& fragmentPath);
        virtual ~VulkanShader();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void SetInt(const std::string& name, int value) const override;
        virtual void SetFloat(const std::string& name, float value) const override;
        virtual void SetVec2(const std::string& name, const glm::vec2& value) const override;
        virtual void SetVec3(const std::string& name, const glm::vec3& value) const override;
        virtual void SetVec4(const std::string& name, const glm::vec4& value) const override;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) const override;

        virtual unsigned int GetID() const override { return 0; } // Unused in Vulkan

        VkShaderModule GetVertexModule() const { return m_VertexModule; }
        VkShaderModule GetFragmentModule() const { return m_FragmentModule; }

    private:
        VkShaderModule CreateShaderModule(const std::vector<char>& code);
        std::vector<char> ReadFile(const std::string& filename);

        VkShaderModule m_VertexModule = VK_NULL_HANDLE;
        VkShaderModule m_FragmentModule = VK_NULL_HANDLE;

        // In a full Vulkan engine, we'd store uniform values here 
        // to be uploaded as Push Constants or Descriptor Sets during Draw.
    };

} // namespace VECTOR
