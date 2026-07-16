#include "Engine/Graphics/Shader.hpp"
#include "Engine/Graphics/RendererAPI.hpp"
#include "Engine/Graphics/OpenGL/OpenGLShader.hpp"
#include "Engine/Graphics/DirectX/DirectX12Shader.hpp"
#include "Engine/Core/Logger.hpp"
#include <fstream>
#include <sstream>

namespace VECTOR {

    std::shared_ptr<Shader> Shader::CreateFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vPath = vertexPath;
        std::string fPath = fragmentPath;

        if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12) {
            // In DX12, we expect a single .hlsl file that contains both vertex and pixel shaders
            // We'll infer the HLSL path from the vertex path by replacing .vert with .hlsl
            size_t pos = vPath.find(".vert");
            if (pos != std::string::npos) {
                vPath.replace(pos, 5, ".hlsl");
            }
            fPath = vPath; // Same file for both
        }

        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            vShaderFile.open(vPath);
            std::stringstream vShaderStream;
            vShaderStream << vShaderFile.rdbuf();
            vShaderFile.close();
            vertexCode = vShaderStream.str();

            if (vPath == fPath) {
                // If it's the same file (HLSL), use the same source code
                fragmentCode = vertexCode;
            } else {
                fShaderFile.open(fPath);
                std::stringstream fShaderStream;
                fShaderStream << fShaderFile.rdbuf();
                fShaderFile.close();
                fragmentCode = fShaderStream.str();
            }
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
            case RendererAPI::API::DirectX12:
                return std::make_shared<DirectX12Shader>(vertexSrc, fragmentSrc);
        }

        VECTOR_LOG_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

} // namespace VECTOR
