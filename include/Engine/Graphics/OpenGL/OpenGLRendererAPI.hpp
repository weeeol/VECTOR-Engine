#pragma once

#include "Engine/Graphics/RendererAPI.hpp"

namespace VECTOR {

    class OpenGLRendererAPI : public RendererAPI {
    public:
        virtual void Init() override;
        virtual void SetClearColor(float r, float g, float b, float a) override;
        virtual void Clear() override;
    };

} // namespace VECTOR
