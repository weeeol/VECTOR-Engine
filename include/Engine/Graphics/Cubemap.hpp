#pragma once

#include <string>
#include <vector>
#include <memory>

namespace VECTOR {

    class Cubemap {
    public:
        virtual ~Cubemap() = default;

        virtual unsigned int GetID() const { return 0; }
        
        static std::shared_ptr<Cubemap> Create(const std::vector<std::string>& faces);
    };

} // namespace VECTOR
