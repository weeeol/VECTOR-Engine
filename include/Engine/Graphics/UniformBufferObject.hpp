#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <cstdint>

namespace VECTOR {

    /**
     * @struct PerFrameData
     * @brief Data uploaded to the GPU once per frame via a Uniform Buffer Object.
     * 
     * Layout matches std140 rules so it maps directly to the GLSL uniform block.
     * All vec3 fields are padded to vec4 for std140 alignment.
     */
    struct PerFrameData {
        glm::mat4 view;              // offset 0,   size 64
        glm::mat4 projection;        // offset 64,  size 64
        glm::mat4 lightSpaceMatrix;  // offset 128, size 64
        glm::vec4 viewPos;           // offset 192, size 16  (vec3 + padding)
        glm::vec4 sunDir;            // offset 208, size 16  (vec3 + padding)
        glm::vec4 sunColor;          // offset 224, size 16  (vec3 + padding)
        glm::vec4 lightPos;          // offset 240, size 16  (vec3 + padding)
        glm::vec4 lightColor;        // offset 256, size 16  (vec3 + padding)
    };
    // Total: 272 bytes

    #define MAX_POINT_LIGHTS 64

    struct PointLightData {
        glm::vec4 positionAndRadius; // xyz = position, w = radius
        glm::vec4 colorAndIntensity; // xyz = color, w = intensity
    };

    struct DirectionalLightData {
        glm::vec4 direction;         // xyz = direction, w = padding
        glm::vec4 colorAndIntensity; // xyz = color, w = intensity
    };

    /**
     * @struct LightUBOData
     * @brief Data uploaded to GPU containing multi-light info via a UBO.
     */
    struct LightUBOData {
        DirectionalLightData dirLight;
        PointLightData pointLights[MAX_POINT_LIGHTS];
        int numPointLights;
        int padding[3];
    };

    /**
     * @class UniformBuffer
     * @brief Manages a Uniform Buffer Object for uploading structured data to shaders.
     */
    class UniformBuffer {
    public:
        virtual ~UniformBuffer() = default;

        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual uint32_t GetBindingPoint() const = 0;

        static void BindShaderBlock(uint32_t shaderProgramID, const char* blockName, uint32_t bindingPoint);
        static std::unique_ptr<UniformBuffer> Create(uint32_t size, uint32_t bindingPoint);
    };

} // namespace VECTOR
