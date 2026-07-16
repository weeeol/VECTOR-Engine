#pragma once

#include "Engine/Graphics/Texture2D.hpp"
#include <string>

namespace VECTOR {

    class OpenGLTexture2D : public Texture2D {
    public:
        OpenGLTexture2D(const std::string& path);
        virtual ~OpenGLTexture2D();

        virtual void Bind(unsigned int slot = 0) const override;
        virtual void Unbind() const override;

        virtual unsigned int GetID() const override { return m_RendererID; }
        virtual int GetWidth() const override { return m_Width; }
        virtual int GetHeight() const override { return m_Height; }

    private:
        unsigned int m_RendererID;
        int m_Width, m_Height, m_BPP;
        std::string m_FilePath;
    };

} // namespace VECTOR
