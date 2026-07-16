#pragma once

#include "Engine/Graphics/Shader.hpp"

namespace VECTOR {

    class OpenGLShader : public Shader {
    public:
        OpenGLShader(const std::string& vertexSource, const std::string& fragmentSource);
        virtual ~OpenGLShader();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void SetInt(const std::string& name, int value) const override;
        virtual void SetFloat(const std::string& name, float value) const override;
        virtual void SetVec3(const std::string& name, const glm::vec3& value) const override;
        virtual void SetVec4(const std::string& name, const glm::vec4& value) const override;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) const override;

        unsigned int GetID() const { return m_ProgramID; }

    private:
        unsigned int m_ProgramID;

        unsigned int CompileShader(unsigned int type, const std::string& source);
        int GetUniformLocation(const std::string& name) const;
    };

} // namespace VECTOR
