#pragma once

#include "Engine/Graphics/DirectX12/DirectX12Shader.hpp"
#include <directx/d3d12.h>
#include <wrl/client.h>
#include <memory>

namespace VECTOR {

    struct DirectX12PipelineConfig {
        D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
        bool isDepthOnly = false;
        bool wireframe = false;
        UINT numRenderTargets = 1;
        DXGI_FORMAT rtvFormats[8] = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN };
        DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;
        bool blendEnable = false;
        D3D12_BLEND srcBlend = D3D12_BLEND_SRC_ALPHA;
        D3D12_BLEND destBlend = D3D12_BLEND_INV_SRC_ALPHA;
        D3D12_BLEND_OP blendOp = D3D12_BLEND_OP_ADD;
        D3D12_BLEND srcBlendAlpha = D3D12_BLEND_ONE;
        D3D12_BLEND destBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        D3D12_BLEND_OP blendOpAlpha = D3D12_BLEND_OP_ADD;
        bool emptyInputLayout = false;
    };

    class DirectX12Pipeline {
    public:
        DirectX12Pipeline(DirectX12Shader* shader, D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS, bool isDepthOnly = false, bool wireframe = false);
        DirectX12Pipeline(DirectX12Shader* shader, const DirectX12PipelineConfig& config);
        ~DirectX12Pipeline();

        ID3D12PipelineState* GetPipelineState() const { return m_PipelineState.Get(); }
        ID3D12RootSignature* GetRootSignature() const { return m_RootSignature.Get(); }

    private:
        void CreateRootSignature();
        void CreatePipelineState(DirectX12Shader* shader, D3D12_COMPARISON_FUNC depthFunc, bool isDepthOnly, bool wireframe);
        void CreatePipelineState(DirectX12Shader* shader, const DirectX12PipelineConfig& config);

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
    };

} // namespace VECTOR
