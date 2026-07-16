#include "Engine/Graphics/DirectX/DirectX12Shader.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    DirectX12Shader::DirectX12Shader(const std::string& vertexSource, const std::string& fragmentSource) {
        VECTOR_LOG_INFO("Creating DirectX12Shader (Stub)");
        // TODO: Compile HLSL into m_VertexShaderBlob and m_PixelShaderBlob
    }

    DirectX12Shader::~DirectX12Shader() {
    }

    void DirectX12Shader::Bind() const {
        // Handled by PSO in DX12
    }

    void DirectX12Shader::Unbind() const {
    }

    void DirectX12Shader::SetInt(const std::string& name, int value) const {
    }

    void DirectX12Shader::SetFloat(const std::string& name, float value) const {
    }

    void DirectX12Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
    }

    void DirectX12Shader::SetVec4(const std::string& name, const glm::vec4& value) const {
    }

    void DirectX12Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
    }

    void DirectX12Shader::CompileShader(const std::string& source, const std::string& target, Microsoft::WRL::ComPtr<ID3DBlob>& blob) {
        // TODO: Use D3DCompile
    }

} // namespace VECTOR
