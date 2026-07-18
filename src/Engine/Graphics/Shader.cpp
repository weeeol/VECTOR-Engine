#include "Engine/Graphics/Shader.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLShader.hpp"
#include "Engine/Graphics/Vulkan/VulkanShader.hpp"
#include "Engine/Core/Logger.hpp"
#include <fstream>
#include <sstream>

namespace VECTOR {

    std::shared_ptr<Shader> Shader::CreateFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
        if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan) {
            return std::make_shared<VulkanShader>(vertexPath, fragmentPath);
        }

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

        return CreateFromSource(vertexCode, fragmentCode);
    }

    std::shared_ptr<Shader> Shader::CreateFromSource(const std::string& vertexSrc, const std::string& fragmentSrc) {
        switch (RendererAPI::GetAPI()) {
            case RendererAPI::API::None:
                VECTOR_LOG_ERROR("RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLShader>(vertexSrc, fragmentSrc);
            case RendererAPI::API::Vulkan:
                VECTOR_LOG_ERROR("VulkanShader doesn't support CreateFromSource (requires SPIR-V file path)!");
                return nullptr;
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
