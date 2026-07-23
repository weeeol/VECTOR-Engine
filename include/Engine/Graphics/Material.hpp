#pragma once

#include <memory>
#include <cstdint>
#include <glm/glm.hpp>

namespace VECTOR {

    class Shader;
    class Texture2D;

    /**
     * @class Material
     * @brief Encapsulates visual surface properties: shader, textures, colors.
     *
     * Materials are designed to be shared across multiple entities that look the same.
     * They provide a sort key so the render queue can batch draw calls by shader/texture.
     */
    class Material {
    public:
        Material();
        ~Material() = default;

        // The shader program used to render this material
        std::shared_ptr<Shader> shader;

        // Optional albedo (diffuse) texture
        std::shared_ptr<Texture2D> albedoTexture;
        
        // PBR textures
        std::shared_ptr<Texture2D> normalTexture;
        std::shared_ptr<Texture2D> metallicRoughnessTexture;
        std::shared_ptr<Texture2D> aoTexture;

        // Base color (multiplied with texture if present)
        glm::vec4 albedoColor = glm::vec4(1.0f);

        // Lighting parameters (PBR)
        float roughness = 0.5f;
        float metallic = 0.0f;

        // If true, skip lighting calculations entirely
        bool isUnlit = false;

        /**
         * @brief Bind this material's shader and set all material-specific uniforms.
         * Does NOT set per-frame uniforms (view, projection, lights) — those come from the UBO.
         * Does NOT set per-object uniforms (model matrix) — that's set per draw call.
         */
        void Bind() const;

        /**
         * @brief Unbind the shader.
         */
        void Unbind() const;

        /**
         * @brief Generate a sort key for draw call batching.
         * High bits = shader program ID, low bits = texture ID.
         * Sorting by this key minimizes state changes.
         */
        uint64_t GetSortKey() const;
    };

} // namespace VECTOR
