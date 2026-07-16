#include "Shader.hpp"
#include "Engine/Core/Logger.hpp"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>

namespace VECTOR {

    Shader::Shader(const std::string& vertexSource, const std::string& fragmentSource) {
        m_ProgramID = glCreateProgram();
        unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
        unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

        glAttachShader(m_ProgramID, vs);
        glAttachShader(m_ProgramID, fs);
        glLinkProgram(m_ProgramID);

        int result;
        glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &result);
        if (result == GL_FALSE) {
            int length;
            glGetProgramiv(m_ProgramID, GL_INFO_LOG_LENGTH, &length);
            char* message = (char*)alloca(length * sizeof(char));
            glGetProgramInfoLog(m_ProgramID, length, &length, message);
            VECTOR_LOG_ERROR(std::string("Failed to link shader program! ") + message);
        }

        glValidateProgram(m_ProgramID);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    Shader::~Shader() {
        glDeleteProgram(m_ProgramID);
    }

    void Shader::Bind() const {
        glUseProgram(m_ProgramID);
    }

    void Shader::Unbind() const {
        glUseProgram(0);
    }

    void Shader::SetInt(const std::string& name, int value) const {
        glUniform1i(GetUniformLocation(name), value);
    }

    void Shader::SetFloat(const std::string& name, float value) const {
        glUniform1f(GetUniformLocation(name), value);
    }

    void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
    }

    void Shader::SetVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
    }

    void Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

    int Shader::GetUniformLocation(const std::string& name) const {
        int location = glGetUniformLocation(m_ProgramID, name.c_str());
        if (location == -1) {
            VECTOR_LOG_WARN("Uniform '" + name + "' doesn't exist!");
        }
        return location;
    }

    unsigned int Shader::CompileShader(unsigned int type, const std::string& source) {
        unsigned int id = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        int result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE) {
            int length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            char* message = (char*)alloca(length * sizeof(char));
            glGetShaderInfoLog(id, length, &length, message);
            VECTOR_LOG_ERROR(std::string("Failed to compile shader! ") + message);
            glDeleteShader(id);
            return 0;
        }

        return id;
    }

    std::shared_ptr<Shader> Shader::CreateFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();

            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        } catch (std::ifstream::failure& e) {
            VECTOR_LOG_ERROR("ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " + std::string(e.what()));
            return nullptr;
        }

        return std::make_shared<Shader>(vertexCode, fragmentCode);
    }

    std::shared_ptr<Shader> Shader::CreateFromSource(const std::string& vertexSrc, const std::string& fragmentSrc) {
        return std::make_shared<Shader>(vertexSrc, fragmentSrc);
    }

} // namespace VECTOR
