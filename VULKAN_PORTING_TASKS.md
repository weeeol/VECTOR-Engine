# Vulkan Porting Tasks

This document tracks recent rendering features that were successfully implemented and stabilized in the DirectX 12 backend (the primary target) but have not yet been ported to the Vulkan backend. 

## 1. Image-Based Lighting (IBL)

**Status:** Implemented in DX12. Missing in Vulkan.

### Details
In DirectX 12, the `main3D.hlsl` shader dynamically samples the environment's skybox cubemap to compute ambient lighting, creating realistic reflections on metallic objects and soft irradiance on diffuse objects.

### Porting Checklist for Vulkan
- [ ] **Shader Binding:** In `assets/engine/shaders/vulkan/main3D.frag`, add a new binding for the skybox cubemap (e.g., `layout(set = 0, binding = 4) uniform samplerCube skybox;`).
- [ ] **Descriptor Set Layout:** Update the Vulkan descriptor set layouts in `VulkanRenderer.cpp` or `VulkanPipeline.cpp` to allocate and accept a `samplerCube` at the new binding index.
- [ ] **Descriptor Updates:** In `VulkanRenderer::FlushMainPass()` (or wherever global sets are bound), bind the current `VulkanCubemap`'s image view and sampler to the descriptor set.
- [ ] **Shader Math Translation:** Translate the HLSL IBL approximation logic into GLSL inside `main3D.frag`. Replace the hardcoded `vec3 ambient = vec3(0.03) * albedo * ao * ssao;` with the Split-Sum approximation (Fresnel Schlick, mip-level selection based on roughness, etc.).

*Note: The `PerFrameData` struct in both `UniformBufferObject.hpp` and `main3D.frag` was already padded with a `skyboxIndex` to maintain parity with DX12's buffer sizing, though Vulkan will likely rely on standard descriptor binding rather than a bindless index.*

## 2. Wireframe Unlit Debug Mode

**Status:** Implemented in DX12. Missing in Vulkan.

### Details
In DirectX 12, wireframe/debug mode renders objects as unlit because PBR IBL perfectly reflects the skybox, effectively camouflaging metallic meshes against the sky background.

### Porting Checklist for Vulkan
- [ ] Update `main3D.frag` `PerFrameData` uniform block to include `debugMode` matching the C++ struct.
- [ ] In `main3D.frag`, if `pfd.debugMode != 0`, immediately return the raw albedo color without computing PBR lighting.
