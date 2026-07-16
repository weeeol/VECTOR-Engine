#pragma once

#include "Engine/Graphics/Texture2D.hpp"
#include <wrl.h>
#include <d3d12.h>
#include <string>

namespace VECTOR {

    class DirectX12Texture2D : public Texture2D {
    public:
        DirectX12Texture2D(const std::string& path);
        virtual ~DirectX12Texture2D();

        virtual void Bind(unsigned int slot = 0) const override;
        virtual void Unbind() const override;

        virtual unsigned int GetID() const override { return 0; } // IDs are an OpenGL concept
        virtual int GetWidth() const override { return m_Width; }
        virtual int GetHeight() const override { return m_Height; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;
        int m_Width, m_Height, m_BPP;
        std::string m_FilePath;
    };

} // namespace VECTOR
