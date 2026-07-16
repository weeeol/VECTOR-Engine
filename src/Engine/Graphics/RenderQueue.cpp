#include "RenderQueue.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"

namespace VECTOR {

    void RenderQueue::Submit(const RenderCommand& cmd) {
        m_Commands.push_back(cmd);
    }

    void RenderQueue::SortAndFlush() {
        // Sort by sort key to minimize shader/texture state changes
        std::sort(m_Commands.begin(), m_Commands.end(),
            [](const RenderCommand& a, const RenderCommand& b) {
                return a.sortKey < b.sortKey;
            }
        );

        m_LastDrawCallCount = 0;
        uint64_t lastSortKey = UINT64_MAX; // Force first bind

        for (const auto& cmd : m_Commands) {
            if (!cmd.mesh) continue;

            // Only re-bind material when the sort key changes
            if (cmd.sortKey != lastSortKey) {
                if (cmd.material) {
                    cmd.material->Bind();
                }
                lastSortKey = cmd.sortKey;
            }

            // Set per-object uniform (model matrix)
            if (cmd.material && cmd.material->shader) {
                cmd.material->shader->SetMat4("model", cmd.modelMatrix);
            }

            cmd.mesh->Draw();
            m_LastDrawCallCount++;
        }

        // Unbind the last material
        if (!m_Commands.empty() && m_Commands.back().material) {
            m_Commands.back().material->Unbind();
        }

        m_Commands.clear();
    }

    void RenderQueue::Clear() {
        m_Commands.clear();
    }

} // namespace VECTOR
