#pragma once

#include "Engine/Graphics/Shader.hpp"
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace VECTOR {

    class OpenGLShader : public Shader {
    public:
        OpenGLShader(const std::string& vertexSource, const std::string& fragmentSource);
        ~OpenGLShader() override;

        void Bind() const override;
        void Unbind() const override;

        void SetInt(const std::string& name, int value) const override;
        void SetFloat(const std::string& name, float value) const override;
        void SetVec2(const std::string& name, const glm::vec2& value) const override;
        void SetVec3(const std::string& name, const glm::vec3& value) const override;
        void SetVec4(const std::string& name, const glm::vec4& value) const override;
        void SetMat4(const std::string& name, const glm::mat4& value) const override;

        unsigned int GetID() const override { return m_ProgramID; }

    private:
        unsigned int m_ProgramID;
        mutable std::unordered_map<std::string, int> m_UniformLocationCache;

        unsigned int CompileShader(unsigned int type, const std::string& source);
        int GetUniformLocation(const std::string& name) const;
    };

} // namespace VECTOR
