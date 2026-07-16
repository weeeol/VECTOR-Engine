#pragma once

#include <GL/glew.h>
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
     * @brief Manages a GL Uniform Buffer Object for uploading structured data to shaders.
     * 
     * Bind this UBO to a binding point (e.g., 0) and declare a matching uniform block
     * in GLSL with `layout(std140, binding = 0)`.
     */
    class UniformBuffer {
    public:
        UniformBuffer(uint32_t size, uint32_t bindingPoint);
        ~UniformBuffer();

        UniformBuffer(const UniformBuffer&) = delete;
        UniformBuffer& operator=(const UniformBuffer&) = delete;

        /**
         * @brief Upload data to the UBO. Size must match the buffer size.
         */
        void SetData(const void* data, uint32_t size, uint32_t offset = 0);

        /**
         * @brief Bind the UBO to its binding point.
         */
        void Bind() const;

        /**
         * @brief Unbind the UBO.
         */
        void Unbind() const;

        uint32_t GetBindingPoint() const { return m_BindingPoint; }

        /**
         * @brief Bind a shader's uniform block to this UBO's binding point.
         * Call this once per shader that uses the uniform block.
         */
        static void BindShaderBlock(uint32_t shaderProgramID, const char* blockName, uint32_t bindingPoint);

    private:
        GLuint m_UBO = 0;
        uint32_t m_BindingPoint = 0;
    };

} // namespace VECTOR
