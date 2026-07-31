#include "Engine/Graphics/DirectX12/DirectX12Shader.hpp"
#include "Engine/Core/Logger.hpp"
#include <fstream>
#include <sstream>

namespace VECTOR {

    DirectX12Shader::DirectX12Shader(const std::string& vertexPath, const std::string& fragmentPath) {
        auto loadFile = [](const std::string& path) {
            std::ifstream file(path);
            std::stringstream buffer;
            if (file) {
                buffer << file.rdbuf();
            } else {
                VECTOR_LOG_ERROR("Could not open shader file: " + path);
            }
            return buffer.str();
        };

        Compile(loadFile(vertexPath), L"VSMain", L"vs_6_6", m_VertexBlob);
        Compile(loadFile(fragmentPath), L"PSMain", L"ps_6_6", m_PixelBlob);
    }

    DirectX12Shader::DirectX12Shader(const std::string& vertexSrc, const std::string& fragmentSrc, bool isSource) {
        Compile(vertexSrc, L"VSMain", L"vs_6_6", m_VertexBlob);
        Compile(fragmentSrc, L"PSMain", L"ps_6_6", m_PixelBlob);
    }

    DirectX12Shader::~DirectX12Shader() {
        m_VertexBlob.Reset();
        m_PixelBlob.Reset();
    }

    void DirectX12Shader::Compile(const std::string& source, const std::wstring& entryPoint, const std::wstring& targetProfile, Microsoft::WRL::ComPtr<IDxcBlob>& outBlob) {
        VECTOR_LOG_INFO("DirectX12Shader::Compile started for profile...");
        Microsoft::WRL::ComPtr<IDxcUtils> utils;
        Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
        
        HRESULT hr1 = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
        if (FAILED(hr1)) { VECTOR_LOG_ERROR("Failed to create DxcUtils"); return; }
        
        HRESULT hr2 = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
        if (FAILED(hr2)) { VECTOR_LOG_ERROR("Failed to create DxcCompiler"); return; }

        VECTOR_LOG_INFO("DxcUtils and DxcCompiler created.");
        
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
        utils->CreateDefaultIncludeHandler(&includeHandler);

        Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
        utils->CreateBlob(source.c_str(), (uint32_t)source.size(), CP_UTF8, &sourceBlob);

        if (!sourceBlob) { VECTOR_LOG_ERROR("sourceBlob is null! Source size: " + std::to_string(source.size())); return; }

        std::vector<LPCWSTR> arguments = {
            L"-E", entryPoint.c_str(),
            L"-T", targetProfile.c_str(),
            L"-Zi", // Enable debug info
            L"-Qstrip_reflect",
            L"-Qstrip_debug"
        };

        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
        sourceBuffer.Size = sourceBlob->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_ACP;

        Microsoft::WRL::ComPtr<IDxcResult> result;
        HRESULT hr = compiler->Compile(
            &sourceBuffer,
            arguments.data(),
            (uint32_t)arguments.size(),
            includeHandler.Get(),
            IID_PPV_ARGS(&result)
        );

        if (SUCCEEDED(hr)) {
            result->GetStatus(&hr);
        }

        if (FAILED(hr)) {
            if (result) {
                Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
                result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
                if (errors && errors->GetStringLength() > 0) {
                    VECTOR_LOG_ERROR("Shader Compilation Error: " + std::string((char*)errors->GetStringPointer()));
                }
            }
            return;
        }

        result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&outBlob), nullptr);
        VECTOR_LOG_INFO("DirectX12Shader::Compile finished successfully.");
    }

} // namespace VECTOR
