#include "Material.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"

namespace VECTOR {

    Material::Material() = default;

    void Material::Bind() const {
        if (shader) {
            shader->Bind();

            // Material-specific uniforms
            shader->SetVec4("material.albedoColor", albedoColor);
            shader->SetFloat("material.specularStrength", specularStrength);
            shader->SetFloat("material.shininess", shininess);
            shader->SetInt("material.isUnlit", isUnlit ? 1 : 0);

            if (albedoTexture) {
                albedoTexture->Bind(0);
                shader->SetInt("material.diffuseTexture", 0);
                shader->SetInt("material.hasTexture", 1);
            } else {
                shader->SetInt("material.hasTexture", 0);
            }
        }
    }

    void Material::Unbind() const {
        if (albedoTexture) {
            albedoTexture->Unbind();
        }
        if (shader) {
            shader->Unbind();
        }
    }

    uint64_t Material::GetSortKey() const {
        uint32_t shaderID = shader ? shader->GetID() : 0;
        uint32_t textureID = albedoTexture ? albedoTexture->GetID() : 0;
        return (static_cast<uint64_t>(shaderID) << 32) | static_cast<uint64_t>(textureID);
    }

} // namespace VECTOR
