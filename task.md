# OpenGL Improvements — Task List

## Phase 1: Material System + Render Queue + UBO
- [x] Create `Material.hpp` / `Material.cpp`
- [x] Create `RenderQueue.hpp` / `RenderQueue.cpp`
- [x] Create `UniformBufferObject.hpp` / `UniformBufferObject.cpp`
- [x] Update `Components.hpp` — RenderComponent uses Material
- [x] Update `Renderer.hpp` / `Renderer.cpp` — integrate queue, UBO, materials
- [x] Update shaders (`main3D.vert`, `main3D.frag`, `depth.vert`) for UBO
- [x] Update `GameplayScene.cpp` — adapt to Material API
- [x] Update `ShootingSystem.cpp` — adapt to Material API
- [x] Update `CMakeLists.txt` — add new source files
- [x] Build and verify (0 errors, 0 new warnings)

## Phase 2: HDR + Bloom
- [ ] Modify `SetupFBOs()` for HDR (`GL_RGBA16F`) + bloom FBOs
- [ ] Create `bloom_downsample.frag` and `bloom_upsample.frag`
- [ ] Update `postprocess.frag` with tone mapping + bloom combine
- [ ] Update Renderer bloom pass logic
- [ ] Build and verify

## Phase 3: Frustum Culling + Multi-Light
- [ ] Create `Frustum.hpp` / `Frustum.cpp`
- [ ] Add light components (`PointLightComponent`, `DirectionalLightComponent`)
- [ ] Update Renderer for frustum culling before submit
- [ ] Update shaders for N-light loop via UBO/SSBO
- [ ] Update `GameplayScene.cpp` to use light entities
- [ ] Build and verify
