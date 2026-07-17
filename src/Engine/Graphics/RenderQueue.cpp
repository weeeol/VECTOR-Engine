#include "Engine/Graphics/RenderQueue.hpp"
#include "Engine/Graphics/Material.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Shader.hpp"

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
        const Material* lastMaterial = nullptr;
        const Shader* currentShader = nullptr;

        for (const auto& cmd : m_Commands) {
            if (!cmd.mesh) continue;

            // SortKey just groups them, but we still need to bind different materials!
            if (cmd.material && cmd.material != lastMaterial) {
                if (cmd.material->shader.get() != currentShader) {
                    currentShader = cmd.material->shader.get();
                    if (currentShader) {
                        currentShader->Bind();
                        currentShader->SetInt("shadowMap", 1);
                    }
                }
                cmd.material->Bind();
                lastMaterial = cmd.material;
            }

            // Set per-object uniform (model matrix)
            if (currentShader) {
                currentShader->SetMat4("model", cmd.modelMatrix);
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
