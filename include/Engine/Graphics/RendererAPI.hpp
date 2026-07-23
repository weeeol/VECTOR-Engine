#pragma once

#include <memory>

namespace VECTOR {

    class RendererAPI {
    public:
        enum class API {
            None = 0, OpenGL = 1, Vulkan = 2
        };

        inline static API GetAPI() { return s_API; }
        inline static void SetAPI(API api) { s_API = api; }

    private:
        static API s_API;
    };

} // namespace VECTOR
