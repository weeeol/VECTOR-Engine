#include "Engine/Graphics/RenderQueue.hpp"
#include "Engine/Graphics/Material.hpp"
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Shader.hpp"
#include "Engine/Graphics/Texture2D.hpp"

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

                if (cmd.boneTransforms && !cmd.boneTransforms->empty()) {
                    for (int i = 0; i < std::min((int)cmd.boneTransforms->size(), 100); i++) {
                        currentShader->SetMat4("finalBonesMatrices[" + std::to_string(i) + "]", (*cmd.boneTransforms)[i]);
                    }
                }
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

    void RenderQueue::SortAndFlushGBuffer(Shader* gBufferShader) {
        std::sort(m_Commands.begin(), m_Commands.end(),
            [](const RenderCommand& a, const RenderCommand& b) {
                return a.sortKey < b.sortKey;
            }
        );

        m_LastDrawCallCount = 0;
        
        if (gBufferShader) {
            gBufferShader->Bind();
        }

        for (const auto& cmd : m_Commands) {
            if (!cmd.mesh) continue;

            if (cmd.material) {
                gBufferShader->SetVec4("material.albedoColor", cmd.material->albedoColor);
                gBufferShader->SetFloat("material.roughness", cmd.material->roughness);
                gBufferShader->SetFloat("material.metallic", cmd.material->metallic);
                gBufferShader->SetInt("material.isUnlit", cmd.material->isUnlit ? 1 : 0);

                if (cmd.material->albedoTexture) {
                    cmd.material->albedoTexture->Bind(0);
                    gBufferShader->SetInt("material.diffuseTexture", 0);
                    gBufferShader->SetInt("material.hasTexture", 1);
                } else {
                    gBufferShader->SetInt("material.hasTexture", 0);
                }
            }

            if (gBufferShader) {
                gBufferShader->SetMat4("model", cmd.modelMatrix);
                
                if (cmd.boneTransforms && !cmd.boneTransforms->empty()) {
                    for (int i = 0; i < std::min((int)cmd.boneTransforms->size(), 100); i++) {
                        gBufferShader->SetMat4("finalBonesMatrices[" + std::to_string(i) + "]", (*cmd.boneTransforms)[i]);
                    }
                }
            }

            cmd.mesh->Draw();
            m_LastDrawCallCount++;
        }

        for (const auto& cmd : m_Commands) {
            if (cmd.material && cmd.material->albedoTexture) {
                cmd.material->albedoTexture->Unbind();
            }
        }

        m_Commands.clear();
    }

    void RenderQueue::Clear() {
        m_Commands.clear();
    }

} // namespace VECTOR
