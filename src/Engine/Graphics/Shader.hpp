#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>

namespace VECTOR {

    class Shader {
    public:
        Shader(const std::string& vertexSource, const std::string& fragmentSource);
        ~Shader();

        void Bind() const;
        void Unbind() const;

        void SetInt(const std::string& name, int value) const;
        void SetFloat(const std::string& name, float value) const;
        void SetVec3(const std::string& name, const glm::vec3& value) const;
        void SetVec4(const std::string& name, const glm::vec4& value) const;
        void SetMat4(const std::string& name, const glm::mat4& value) const;

        unsigned int GetID() const { return m_ProgramID; }

        // Utility to load from file
        static std::shared_ptr<Shader> CreateFromFile(const std::string& vertexPath, const std::string& fragmentPath);
        static std::shared_ptr<Shader> CreateFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);

    private:
        unsigned int m_ProgramID;

        unsigned int CompileShader(unsigned int type, const std::string& source);
        int GetUniformLocation(const std::string& name) const;
    };

} // namespace VECTOR
