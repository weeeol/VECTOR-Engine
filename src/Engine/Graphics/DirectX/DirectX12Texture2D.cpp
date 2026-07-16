#include "Engine/Graphics/DirectX/DirectX12Texture2D.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    DirectX12Texture2D::DirectX12Texture2D(const std::string& path)
        : m_FilePath(path), m_Width(0), m_Height(0), m_BPP(0) {
        VECTOR_LOG_INFO("Creating DirectX12Texture2D from " + path + " (Stub)");
        // TODO: Use SDL_image or stbi to load pixels, then upload to D3D12Resource
    }

    DirectX12Texture2D::~DirectX12Texture2D() {
    }

    void DirectX12Texture2D::Bind(unsigned int slot) const {
        // Will be handled via Descriptor Tables and Root Signatures in DX12
    }

    void DirectX12Texture2D::Unbind() const {
    }

} // namespace VECTOR
