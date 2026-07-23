#pragma once

#include "Engine/ECS/ECS.hpp"
#include <string>

namespace VECTOR {

    class SceneSerializer {
    public:
        SceneSerializer(Registry* registry);

        void Serialize(const std::string& filepath);
        bool Deserialize(const std::string& filepath);

    private:
        Registry* m_Registry;
    };

}
