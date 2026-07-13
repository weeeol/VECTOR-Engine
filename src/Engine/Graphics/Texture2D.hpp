#pragma once

#include <string>
#include <memory>

namespace VECTOR {

    class Texture2D {
    public:
        Texture2D(const std::string& path);
        ~Texture2D();

        void Bind(unsigned int slot = 0) const;
        void Unbind() const;

        unsigned int GetID() const { return m_RendererID; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

        static std::shared_ptr<Texture2D> Create(const std::string& path);

    private:
        unsigned int m_RendererID;
        int m_Width, m_Height, m_BPP;
        std::string m_FilePath;
    };

} // namespace VECTOR
