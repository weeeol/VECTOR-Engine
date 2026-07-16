#pragma once

#include "Engine/Graphics/Shader.hpp"
#include <wrl.h>
#include <d3d12.h>
#include <d3dcompiler.h>

namespace VECTOR {

    class DirectX12Shader : public Shader {
    public:
        DirectX12Shader(const std::string& vertexSource, const std::string& fragmentSource);
        virtual ~DirectX12Shader();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        ID3D12PipelineState* GetPipelineState() const { return m_PipelineState.Get(); }

        virtual void SetInt(const std::string& name, int value) const override;
        virtual void SetFloat(const std::string& name, float value) const override;
        virtual void SetVec3(const std::string& name, const glm::vec3& value) const override;
        virtual void SetVec4(const std::string& name, const glm::vec4& value) const override;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) const override;

    private:
        Microsoft::WRL::ComPtr<ID3DBlob> m_VertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> m_PixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
        
        void CompileShader(const std::string& source, const std::string& target, Microsoft::WRL::ComPtr<ID3DBlob>& blob);
    };

} // namespace VECTOR
