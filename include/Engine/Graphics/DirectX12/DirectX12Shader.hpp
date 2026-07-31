#pragma once

#include "Engine/Graphics/Shader.hpp"
#include <directx/d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <string>

namespace VECTOR {

    class DirectX12Shader : public Shader {
    public:
        DirectX12Shader(const std::string& vertexPath, const std::string& fragmentPath);
        DirectX12Shader(const std::string& vertexSrc, const std::string& fragmentSrc, bool isSource);
        virtual ~DirectX12Shader();

        virtual void Bind() const override {}
        virtual void Unbind() const override {}

        virtual void SetInt(const std::string& name, int value) const override {}
        virtual void SetFloat(const std::string& name, float value) const override {}
        virtual void SetVec2(const std::string& name, const glm::vec2& value) const override {}
        virtual void SetVec3(const std::string& name, const glm::vec3& value) const override {}
        virtual void SetVec4(const std::string& name, const glm::vec4& value) const override {}
        virtual void SetMat4(const std::string& name, const glm::mat4& value) const override {}

        virtual unsigned int GetID() const override { return 0; }

        IDxcBlob* GetVertexBlob() const { return m_VertexBlob.Get(); }
        IDxcBlob* GetPixelBlob() const { return m_PixelBlob.Get(); }

    private:
        void Compile(const std::string& source, const std::wstring& entryPoint, const std::wstring& targetProfile, Microsoft::WRL::ComPtr<IDxcBlob>& outBlob);

    private:
        Microsoft::WRL::ComPtr<IDxcBlob> m_VertexBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> m_PixelBlob;
    };

} // namespace VECTOR
