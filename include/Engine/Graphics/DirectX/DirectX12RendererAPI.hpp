#pragma once

#include "Engine/Graphics/RendererAPI.hpp"

namespace VECTOR {

    class DirectX12RendererAPI : public RendererAPI {
    public:
        virtual void Init() override;
        virtual void SetClearColor(float r, float g, float b, float a) override;
        virtual void Clear() override;
        
    private:
        float m_ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

} // namespace VECTOR
