#include "Engine/Graphics/DirectX/DirectX12Shader.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/DirectX/DirectX12Context.hpp"

namespace VECTOR {

    DirectX12Shader::DirectX12Shader(const std::string& vertexSource, const std::string& fragmentSource) {
        VECTOR_LOG_INFO("Compiling DirectX12Shader");
        CompileShader(vertexSource, "vs_5_0", m_VertexShaderBlob);
        CompileShader(fragmentSource, "ps_5_0", m_PixelShaderBlob);
        
        // Create Pipeline State Object (PSO)
        auto context = DirectX12Context::Get();
        if (!context || !context->GetRootSignature() || !m_VertexShaderBlob || !m_PixelShaderBlob) {
            VECTOR_LOG_ERROR("Failed to create PSO: Context, Root Signature, or Shaders are invalid.");
            return;
        }

        // Determine input layout based on shader content (heuristics)
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
        if (vertexSource.find("NORMAL") != std::string::npos || vertexSource.find("normal") != std::string::npos) {
            // Mesh3D: pos(3), normal(3), tex(2)
            inputLayout = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
        } else {
            // Quad2D: pos(2), tex(2)
            inputLayout = {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
        psoDesc.pRootSignature = context->GetRootSignature();
        psoDesc.VS = { m_VertexShaderBlob->GetBufferPointer(), m_VertexShaderBlob->GetBufferSize() };
        
        // Depth shader might not have a pixel shader!
        if (fragmentSource.empty() || fragmentSource.find("SV_TARGET") == std::string::npos) {
            psoDesc.PS = { nullptr, 0 };
        } else {
            psoDesc.PS = { m_PixelShaderBlob->GetBufferPointer(), m_PixelShaderBlob->GetBufferSize() };
        }

        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // We disable culling for simplicity
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // Basic blending for transparency
        D3D12_RENDER_TARGET_BLEND_DESC blendDesc = {};
        blendDesc.BlendEnable = TRUE;
        blendDesc.LogicOpEnable = FALSE;
        blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
        blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
            psoDesc.BlendState.RenderTarget[i] = blendDesc;
        }

        if (vertexSource.find("main2D") != std::string::npos || vertexSource.find("postprocess") != std::string::npos) {
            psoDesc.DepthStencilState.DepthEnable = FALSE;
            psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        } else {
            psoDesc.DepthStencilState.DepthEnable = TRUE;
            psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        }
        
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        
        // Depth only pass vs color pass
        if (psoDesc.PS.pShaderBytecode == nullptr) {
            psoDesc.NumRenderTargets = 0;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
        } else {
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.SampleDesc.Count = 1;

        if (FAILED(context->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState)))) {
            VECTOR_LOG_ERROR("Failed to create Pipeline State Object.");
        }
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
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompile(
            source.c_str(),
            source.size(),
            nullptr,
            nullptr,
            nullptr,
            target == "vs_5_0" ? "VSMain" : "PSMain",
            target.c_str(),
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
            0,
            &blob,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                VECTOR_LOG_ERROR("Shader Compilation Failed: " + std::string((char*)errorBlob->GetBufferPointer()));
            } else {
                VECTOR_LOG_ERROR("Shader Compilation Failed with no error message.");
            }
        }
    }

} // namespace VECTOR
