#include "Engine/Graphics/DirectX12/DirectX12Pipeline.hpp"
#include "Engine/Graphics/DirectX12/DirectX12Context.hpp"
#include "Engine/Core/Logger.hpp"
#include <directx/d3dx12.h>
#include <string>

namespace VECTOR {

    DirectX12Pipeline::DirectX12Pipeline(DirectX12Shader* shader, D3D12_COMPARISON_FUNC depthFunc, bool isDepthOnly, bool wireframe) {
        VECTOR_LOG_INFO("DirectX12Pipeline: Creating Root Signature...");
        CreateRootSignature();
        VECTOR_LOG_INFO("DirectX12Pipeline: Creating Pipeline State...");
        CreatePipelineState(shader, depthFunc, isDepthOnly, wireframe);
        VECTOR_LOG_INFO("DirectX12Pipeline: Creation finished.");
    }

    DirectX12Pipeline::DirectX12Pipeline(DirectX12Shader* shader, const DirectX12PipelineConfig& config) {
        VECTOR_LOG_INFO("DirectX12Pipeline: Creating Root Signature...");
        CreateRootSignature();
        VECTOR_LOG_INFO("DirectX12Pipeline: Creating Pipeline State...");
        CreatePipelineState(shader, config);
        VECTOR_LOG_INFO("DirectX12Pipeline: Creation finished.");
    }

    DirectX12Pipeline::~DirectX12Pipeline() {
        m_PipelineState.Reset();
        m_RootSignature.Reset();
    }

    void DirectX12Pipeline::CreateRootSignature() {
        auto device = DirectX12Context::Get()->GetDevice();

        // SM 6.6 Bindless allows shaders to index directly into the descriptor heap.
        // We only need root constants to pass indices to the shader (e.g., an object ID or material ID).
        D3D12_ROOT_PARAMETER1 rootParameters[4];
        
        // b0: PerFrameData
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        rootParameters[0].Descriptor.RegisterSpace = 0;
        rootParameters[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // b1: LightDataBlock
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].Descriptor.ShaderRegister = 1;
        rootParameters[1].Descriptor.RegisterSpace = 0;
        rootParameters[1].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // b2: PerObjectData
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[2].Descriptor.ShaderRegister = 2;
        rootParameters[2].Descriptor.RegisterSpace = 0;
        rootParameters[2].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // b3: MaterialData
        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[3].Descriptor.ShaderRegister = 3;
        rootParameters[3].Descriptor.RegisterSpace = 0;
        rootParameters[3].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
        
        // s0: defaultSampler
        staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].MipLODBias = 0;
        staticSamplers[0].MaxAnisotropy = 16;
        staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSamplers[0].MinLOD = 0.0f;
        staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[0].ShaderRegister = 0;
        staticSamplers[0].RegisterSpace = 0;
        staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // s1: shadowSampler
        staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticSamplers[1].MipLODBias = 0;
        staticSamplers[1].MaxAnisotropy = 1;
        staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        staticSamplers[1].MinLOD = 0.0f;
        staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[1].ShaderRegister = 1;
        staticSamplers[1].RegisterSpace = 0;
        staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSigDesc.Desc_1_1.NumParameters = _countof(rootParameters);
        rootSigDesc.Desc_1_1.pParameters = rootParameters;
        rootSigDesc.Desc_1_1.NumStaticSamplers = _countof(staticSamplers);
        rootSigDesc.Desc_1_1.pStaticSamplers = staticSamplers;
        
        // Key SM 6.6 Flags for Bindless!
        rootSigDesc.Desc_1_1.Flags = 
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
            D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &signature, &error);
        if (FAILED(hr)) {
            if (error) {
                VECTOR_LOG_ERROR("Root Signature Serialization Error: " + std::string((char*)error->GetBufferPointer()));
            }
            return;
        }

        hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create Root Signature.");
        }
    }

    void DirectX12Pipeline::CreatePipelineState(DirectX12Shader* shader, D3D12_COMPARISON_FUNC depthFunc, bool isDepthOnly, bool wireframe) {
        auto device = DirectX12Context::Get()->GetDevice();

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_RootSignature.Get();
        
        if (shader->GetVertexBlob()) {
            psoDesc.VS = CD3DX12_SHADER_BYTECODE(shader->GetVertexBlob()->GetBufferPointer(), shader->GetVertexBlob()->GetBufferSize());
        }
        if (shader->GetPixelBlob() && !isDepthOnly) {
            psoDesc.PS = CD3DX12_SHADER_BYTECODE(shader->GetPixelBlob()->GetBufferPointer(), shader->GetPixelBlob()->GetBufferSize());
        } else {
            psoDesc.PS = { nullptr, 0 };
        }

        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        if (wireframe) {
            psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        }
        if (isDepthOnly) {
            psoDesc.RasterizerState.DepthBias = 10000;
            psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
            psoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
        }

        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthFunc = depthFunc;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        
        if (isDepthOnly) {
            psoDesc.NumRenderTargets = 0;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
        } else {
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create Graphics Pipeline State.");
            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
                UINT64 numMessages = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
                for (UINT64 i = 0; i < numMessages; ++i) {
                    SIZE_T messageLength = 0;
                    infoQueue->GetMessage(i, nullptr, &messageLength);
                    if (messageLength > 0) {
                        D3D12_MESSAGE* message = (D3D12_MESSAGE*)malloc(messageLength);
                        if (SUCCEEDED(infoQueue->GetMessage(i, message, &messageLength))) {
                            VECTOR_LOG_ERROR(std::string("D3D12 Error: ") + message->pDescription);
                        }
                        free(message);
                    }
                }
                infoQueue->ClearStoredMessages();
            }
        }
    }

    void DirectX12Pipeline::CreatePipelineState(DirectX12Shader* shader, const DirectX12PipelineConfig& config) {
        auto device = DirectX12Context::Get()->GetDevice();

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_RootSignature.Get();
        
        if (shader->GetVertexBlob()) {
            psoDesc.VS = CD3DX12_SHADER_BYTECODE(shader->GetVertexBlob()->GetBufferPointer(), shader->GetVertexBlob()->GetBufferSize());
        }
        if (shader->GetPixelBlob() && !config.isDepthOnly) {
            psoDesc.PS = CD3DX12_SHADER_BYTECODE(shader->GetPixelBlob()->GetBufferPointer(), shader->GetPixelBlob()->GetBufferSize());
        } else {
            psoDesc.PS = { nullptr, 0 };
        }

        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        if (config.wireframe) {
            psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        }
        if (config.isDepthOnly) {
            psoDesc.RasterizerState.DepthBias = 10000;
            psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
            psoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
        }

        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        if (!config.blendEnable) {
            // Default blend state (disabled)
        } else {
            psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
            psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }

        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthFunc = config.depthFunc;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        
        if (config.isDepthOnly) {
            psoDesc.NumRenderTargets = 0;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
        } else {
            psoDesc.NumRenderTargets = config.numRenderTargets;
            for (UINT i = 0; i < config.numRenderTargets; ++i) {
                psoDesc.RTVFormats[i] = config.rtvFormats[i];
            }
        }
        
        psoDesc.DSVFormat = config.dsvFormat;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create Graphics Pipeline State with config.");
        }
    }

} // namespace VECTOR
