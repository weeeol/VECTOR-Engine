#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include "Engine/Graphics/Texture2D.hpp"

namespace VECTOR {

class Renderer;

using RGResourceHandle = uint32_t;
constexpr RGResourceHandle RG_INVALID_HANDLE = static_cast<RGResourceHandle>(-1);

enum class RGResourceState {
    UNDEFINED,
    RENDER_TARGET,
    DEPTH_WRITE,
    DEPTH_READ,
    SHADER_RESOURCE,
    UNORDERED_ACCESS,
    PRESENT
};

struct RGTextureDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    TextureFormat format = TextureFormat::RGBA16F;
};

class RenderGraphBuilder {
public:
    virtual ~RenderGraphBuilder() = default;

    virtual RGResourceHandle CreateTexture(const std::string& name, const RGTextureDesc& desc) = 0;
    virtual void ReadTexture(RGResourceHandle handle, RGResourceState state = RGResourceState::SHADER_RESOURCE) = 0;
    virtual void WriteTexture(RGResourceHandle handle, RGResourceState state = RGResourceState::RENDER_TARGET) = 0;
};

class RenderGraphPass {
public:
    virtual ~RenderGraphPass() = default;
    virtual void Execute(Renderer* renderer) = 0;
};

class LambdaRenderPass : public RenderGraphPass {
public:
    LambdaRenderPass(std::function<void(Renderer*)> execute) : m_Execute(execute) {}
    void Execute(Renderer* renderer) override {
        if (m_Execute) m_Execute(renderer);
    }
private:
    std::function<void(Renderer*)> m_Execute;
};

struct RGResourceAccess {
    RGResourceHandle handle;
    RGResourceState state;
};

struct RGPassNode {
    std::string name;
    std::unique_ptr<RenderGraphPass> pass;
    std::vector<RGResourceAccess> reads;
    std::vector<RGResourceAccess> writes;
};

class RenderGraph {
public:
    RenderGraph() = default;
    ~RenderGraph() = default;

    void AddPass(const std::string& name, 
                 std::function<void(RenderGraphBuilder&)> setup, 
                 std::unique_ptr<RenderGraphPass> pass);

    void Compile();
    void Execute(Renderer* renderer);
    void Clear();
    bool IsEmpty() const { return m_Passes.empty(); }

    std::shared_ptr<Texture2D> GetTexture(RGResourceHandle handle) const {
        if (handle >= m_Resources.size()) return nullptr;
        return m_Resources[handle].texture;
    }
    
    std::shared_ptr<Texture2D> GetTexture(const std::string& name) const {
        auto it = m_ResourceNameToHandle.find(name);
        if (it == m_ResourceNameToHandle.end()) return nullptr;
        return GetTexture(it->second);
    }

private:
    class BuilderImpl;
    friend class BuilderImpl;

    std::vector<std::unique_ptr<RGPassNode>> m_Passes;
    std::vector<RGPassNode*> m_ExecutionOrder;
    
    struct ResourceInfo {
        std::string name;
        RGTextureDesc desc;
        RGPassNode* creator = nullptr;
        RGResourceState currentState = RGResourceState::UNDEFINED;
        std::shared_ptr<Texture2D> texture = nullptr;
    };
    std::vector<ResourceInfo> m_Resources;
    std::unordered_map<std::string, RGResourceHandle> m_ResourceNameToHandle;
};

} // namespace VECTOR
