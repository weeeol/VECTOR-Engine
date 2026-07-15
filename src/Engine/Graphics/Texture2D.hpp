#pragma once

#include <string>
#include <memory>

namespace VECTOR {

    class Texture2D {
    public:
        virtual ~Texture2D() = default;

        virtual void Bind(unsigned int slot = 0) const = 0;
        virtual void Unbind() const = 0;

        virtual unsigned int GetID() const = 0;
        virtual int GetWidth() const = 0;
        virtual int GetHeight() const = 0;

        static std::shared_ptr<Texture2D> Create(const std::string& path);
    };

} // namespace VECTOR
