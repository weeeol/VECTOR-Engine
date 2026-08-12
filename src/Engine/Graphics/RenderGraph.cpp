#include "Engine/Graphics/RenderGraph.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Core/Logger.hpp"
#include <algorithm>
#include <stdexcept>

namespace VECTOR {

class RenderGraph::BuilderImpl : public RenderGraphBuilder {
public:
    BuilderImpl(RenderGraph& graph, RGPassNode* currentNode) 
        : m_Graph(graph), m_CurrentNode(currentNode) {}

    RGResourceHandle CreateTexture(const std::string& name, const RGTextureDesc& desc) override {
        if (m_Graph.m_ResourceNameToHandle.find(name) != m_Graph.m_ResourceNameToHandle.end()) {
            VECTOR_LOG_ERROR("RenderGraph: Texture resource '" + name + "' already exists!");
            return RG_INVALID_HANDLE;
        }

        RGResourceHandle handle = static_cast<RGResourceHandle>(m_Graph.m_Resources.size());
        m_Graph.m_Resources.push_back({name, desc, m_CurrentNode, RGResourceState::UNDEFINED});
        m_Graph.m_ResourceNameToHandle[name] = handle;
        
        RGResourceState initialState = (desc.format == TextureFormat::Depth32F) ? RGResourceState::DEPTH_WRITE : RGResourceState::RENDER_TARGET;
        m_CurrentNode->writes.push_back({handle, initialState});
        return handle;
    }

    void ReadTexture(RGResourceHandle handle, RGResourceState state = RGResourceState::SHADER_RESOURCE) override {
        if (handle >= m_Graph.m_Resources.size()) return;
        m_CurrentNode->reads.push_back({handle, state});
    }

    void WriteTexture(RGResourceHandle handle, RGResourceState state = RGResourceState::RENDER_TARGET) override {
        if (handle >= m_Graph.m_Resources.size()) return;
        
        // Update creator if another pass writes to it (e.g. over-writing or chaining)
        m_Graph.m_Resources[handle].creator = m_CurrentNode;
        m_CurrentNode->writes.push_back({handle, state});
    }

private:
    RenderGraph& m_Graph;
    RGPassNode* m_CurrentNode;
};

void RenderGraph::AddPass(const std::string& name, 
                          std::function<void(RenderGraphBuilder&)> setup, 
                          std::unique_ptr<RenderGraphPass> pass) 
{
    auto node = std::make_unique<RGPassNode>();
    node->name = name;
    node->pass = std::move(pass);

    BuilderImpl builder(*this, node.get());
    setup(builder);

    m_Passes.push_back(std::move(node));
}

void RenderGraph::Compile() {
    m_ExecutionOrder.clear();

    std::unordered_map<RGPassNode*, std::vector<RGPassNode*>> adjList;
    std::unordered_map<RGPassNode*, int> inDegree;

    for (auto& node : m_Passes) {
        inDegree[node.get()] = 0;
    }

    for (auto& node : m_Passes) {
        for (const RGResourceAccess& readAccess : node->reads) {
            RGPassNode* producer = m_Resources[readAccess.handle].creator;
            if (producer && producer != node.get()) {
                adjList[producer].push_back(node.get());
                inDegree[node.get()]++;
            }
        }
    }

    std::vector<RGPassNode*> queue;
    for (auto& node : m_Passes) {
        if (inDegree[node.get()] == 0) {
            queue.push_back(node.get());
        }
    }

    while (!queue.empty()) {
        RGPassNode* current = queue.front();
        queue.erase(queue.begin());
        m_ExecutionOrder.push_back(current);

        for (RGPassNode* neighbor : adjList[current]) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                queue.push_back(neighbor);
            }
        }
    }

    if (m_ExecutionOrder.size() != m_Passes.size()) {
        VECTOR_LOG_ERROR("RenderGraph: Cycle detected or disconnected graph components!");
    }

    // 1. Compute resource lifetimes (first used, last used pass index)
    for (auto& res : m_Resources) {
        res.firstPass = -1;
        res.lastPass = -1;
        res.aliasHandle = RG_INVALID_HANDLE;
    }

    for (int i = 0; i < (int)m_ExecutionOrder.size(); ++i) {
        RGPassNode* node = m_ExecutionOrder[i];
        
        auto updateLifetime = [&](RGResourceHandle handle) {
            ResourceInfo& res = m_Resources[handle];
            if (res.firstPass == -1) res.firstPass = i;
            res.lastPass = i;
        };

        for (const auto& readAccess : node->reads) updateLifetime(readAccess.handle);
        for (const auto& writeAccess : node->writes) updateLifetime(writeAccess.handle);
    }

    /*
    // 2. Memory Aliasing: Find resources that don't overlap and match in size/format
    for (size_t i = 0; i < m_Resources.size(); ++i) {
        if (m_Resources[i].firstPass == -1) continue; // Unused resource
        if (m_Resources[i].aliasHandle != RG_INVALID_HANDLE) continue; // Already aliased

        for (size_t j = i + 1; j < m_Resources.size(); ++j) {
            if (m_Resources[j].firstPass == -1 || m_Resources[j].aliasHandle != RG_INVALID_HANDLE) continue;
            
            // Check for overlap
            if (m_Resources[j].firstPass > m_Resources[i].lastPass || m_Resources[i].firstPass > m_Resources[j].lastPass) {
                // Check if they are compatible (size and format)
                if (m_Resources[i].desc.width == m_Resources[j].desc.width &&
                    m_Resources[i].desc.height == m_Resources[j].desc.height &&
                    m_Resources[i].desc.format == m_Resources[j].desc.format) {
                    
                    m_Resources[j].aliasHandle = i;
                    break;
                }
            }
        }
    }
    */
}



void RenderGraph::Execute(Renderer* renderer) {
    for (size_t i = 0; i < m_Resources.size(); ++i) {
        if (!m_Resources[i].texture) {
            Texture2D* aliasTexture = nullptr;
            if (m_Resources[i].aliasHandle != RG_INVALID_HANDLE) {
                aliasTexture = m_Resources[m_Resources[i].aliasHandle].texture.get();
            }
            m_Resources[i].texture = renderer->AllocateTransientTexture(
                static_cast<uint32_t>(i), 
                m_Resources[i].desc.width, 
                m_Resources[i].desc.height, 
                m_Resources[i].desc.format,
                aliasTexture
            );
            m_Resources[i].currentState = RGResourceState::UNDEFINED;
        }
    }

    for (RGPassNode* node : m_ExecutionOrder) {
        std::vector<RGResourceTransition> batchTransitions;

        for (const RGResourceAccess& readAccess : node->reads) {
            ResourceInfo& resInfo = m_Resources[readAccess.handle];
            if (resInfo.currentState != readAccess.state) {
                batchTransitions.push_back({readAccess.handle, static_cast<int>(resInfo.currentState), static_cast<int>(readAccess.state)});
                resInfo.currentState = readAccess.state;
            }
        }

        for (const RGResourceAccess& writeAccess : node->writes) {
            ResourceInfo& resInfo = m_Resources[writeAccess.handle];
            if (resInfo.currentState != writeAccess.state) {
                batchTransitions.push_back({writeAccess.handle, static_cast<int>(resInfo.currentState), static_cast<int>(writeAccess.state)});
                resInfo.currentState = writeAccess.state;
            }
        }

        if (!batchTransitions.empty()) {
            renderer->TransitionResources(batchTransitions);
        }

        if (node->pass) {
            node->pass->Execute(renderer);
        }
    }
}

void RenderGraph::Clear() {
    m_Passes.clear();
    m_ExecutionOrder.clear();
    m_Resources.clear();
    m_ResourceNameToHandle.clear();
}

} // namespace VECTOR
