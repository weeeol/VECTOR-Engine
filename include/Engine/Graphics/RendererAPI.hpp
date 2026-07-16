#pragma once

#include <memory>

namespace VECTOR {

    class RendererAPI {
    public:
        enum class API {
            None = 0, OpenGL = 1, DirectX12 = 2
        };

        virtual ~RendererAPI() = default;

        virtual void Init() = 0;
        virtual void SetClearColor(float r, float g, float b, float a) = 0;
        virtual void Clear() = 0;

        inline static API GetAPI() { return s_API; }
        static void SetAPI(API api) { s_API = api; }

        static RendererAPI* Create();

    private:
        static API s_API;
    };

} // namespace VECTOR
