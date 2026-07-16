# VECTOR Engine — Technology Improvement Roadmap

## Current State Summary

Your engine has solid foundations: a working ECS, Bullet Physics integration, multi-pass OpenGL rendering (shadow maps + post-processing), event bus, scene management, UI system, and audio. Here's where each subsystem can level up.

---

## 🔴 High Impact (Architecture)

### 1. ECS: Sparse Set → Archetype / Data-Oriented

**Current problem**: [ECS.hpp](file:///s:/Projects/VECTOR-Engine/src/Engine/ECS/ECS.hpp) uses a fixed-size `T m_ComponentArray[MAX_ENTITIES]` (5000 elements) with `unordered_map` lookups for every access. `View()` iterates *all* active entities and calls `HasComponent()` per-entity per-type — this is O(N × K) with hash map lookups.

**What to do**:
- **Short term**: Switch to a proper **sparse set** per component type (a dense array + sparse array for O(1) lookup without hashing). This alone eliminates all `unordered_map` overhead.
- **Long term**: Move to an **archetype-based** ECS (like EnTT or Flecs style). Entities with the same set of components are stored contiguously in memory. `View<Transform, Render>()` becomes a tight loop over packed arrays — cache-friendly, vectorizable.
- Add **entity recycling** with a free-list instead of a monotonically incrementing counter (currently [line 73-77](file:///s:/Projects/VECTOR-Engine/src/Engine/ECS/ECS.hpp#L73-L77) — once you hit 5000, you're done).

### 2. Renderer: Monolithic → Command-Based / RenderGraph

**Current problem**: [Renderer.cpp](file:///s:/Projects/VECTOR-Engine/src/Engine/Graphics/Renderer.cpp) (514 lines) is a monolith that directly issues OpenGL calls. Every `DrawMesh()` call sets *all* uniforms (view, projection, lightSpaceMatrix, lightPos, lightColor, etc.) — even if they haven't changed. No batching, no draw call sorting, no instancing.

**What to do**:
- **Render Command Queue**: Instead of immediately calling `glDraw*`, push `DrawCommand` structs (mesh, material, transform, sort key) into a queue. Sort by shader → material → depth before flushing. This minimizes state changes.
- **Material System**: Extract the scattered uniform-setting code (objectColor, lightPos, hasTexture, isUnlit, etc.) into a `Material` class that owns a shader + parameter map. Entities reference a material, not raw shaders.
- **Instanced Rendering**: When multiple entities share the same mesh + material (common in an FPS — e.g. bullets, crates), batch them with `glDrawElementsInstanced`.
- **Frame Graph / Render Graph**: Formalize your passes (shadow → main → post-process) as nodes in a graph with declared inputs/outputs. This makes it trivial to add passes (SSAO, bloom, deferred) without spaghetti.

### 3. Graphics API Abstraction is Incomplete

[RendererAPI.hpp](file:///s:/Projects/VECTOR-Engine/src/Engine/Graphics/RendererAPI.hpp) declares an `API` enum but it's essentially dead code — the `Renderer` class directly calls GL everywhere. Your DX12 log output from the first run suggests you started a DX12 backend at some point.

**What to do**:
- Define a proper **RHI** (Rendering Hardware Interface) with virtual methods: `CreateBuffer`, `CreateTexture`, `CreatePipeline`, `Submit`, etc.
- Implement `OpenGLRHI` (your current code extracted) and `DX12RHI` behind this interface.
- The `Renderer` should only talk to the RHI, never to `GL*` directly.

---

## 🟡 Medium Impact (Features & Quality)

### 4. Resource / Asset Pipeline

[ResourceManager](file:///s:/Projects/VECTOR-Engine/src/Engine/Core/ResourceManager.cpp) loads shaders and fonts synchronously, with paths hardcoded in game code. No model loading (OBJ/glTF).

**What to do**:
- Add **asynchronous asset loading** (load meshes/textures on a worker thread, upload to GPU on the main thread).
- Integrate a **model loader** (Assimp or tinygltf for glTF). Without this, every mesh is constructed procedurally.
- Add **reference counting / handle-based** resource management so assets are auto-released when no entity references them.

### 5. Lighting System

Lighting is completely hardcoded in [Renderer.cpp](file:///s:/Projects/VECTOR-Engine/src/Engine/Graphics/Renderer.cpp#L263-L268): a single point light at `(0, 10, 10)` and a single directional sun at `(-0.2, 1, 0.3)`, baked directly into `DrawMesh()`.

**What to do**:
- Create `PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent` ECS components.
- Collect active lights in the scene, upload to a UBO/SSBO. Support N lights in the shader.
- Later: move to **deferred rendering** (G-buffer) to decouple lighting cost from geometry complexity.

### 6. Fixed-Timestep Game Loop

[Application.cpp](file:///s:/Projects/VECTOR-Engine/src/Engine/Core/Application.cpp#L61-L103) has `accumulator` logic but never uses it — physics and logic both run at variable dt. Your comment says "Bullet handles fixed time steps internally", which is partially true (`stepSimulation` subdivides), but *game logic* still runs at variable rate.

**What to do**:
- Implement a proper fixed-timestep loop: consume `accumulator` in `FIXED_DT`-sized steps for physics + gameplay, interpolate transforms for rendering.
- This prevents non-determinism and physics instability at low/high framerates.

### 7. Post-Processing Pipeline

You have a single post-process pass but it's just a passthrough. The shader files exist but there's room for:
- **HDR rendering** (use `GL_RGBA16F` for the main FBO, add tone mapping)
- **Bloom** (downscale chain + Gaussian blur + additive blend)
- **SSAO** (screen-space ambient occlusion)
- **FXAA/TAA** (anti-aliasing)
- Make these toggleable via a `PostProcessStack` configuration.

---

## 🟢 Lower Priority but Valuable

### 8. ECS System Scheduling

[System.hpp](file:///s:/Projects/VECTOR-Engine/src/Engine/ECS/System.hpp) has a single `Update()`. There's no engine-managed system ordering or parallel execution.

**What to do**:
- Add a `SystemScheduler` that registers systems with declared read/write component access and executes independent systems in parallel.
- Add `Init()`, `FixedUpdate()`, `LateUpdate()`, `OnDestroy()` lifecycle hooks.

### 9. Proper Logging & Debug Tools

[Logger](file:///s:/Projects/VECTOR-Engine/src/Engine/Core/Logger.cpp) (~900 bytes) is minimal. For a serious engine:
- Add log categories/channels (Graphics, Physics, Audio, Game)
- Add log levels with runtime filtering
- Consider integrating **spdlog** (header-only, fast, production-proven)
- Add an **in-engine debug console** and **profiling overlay** (draw call count, frame time graph)

### 10. Serialization / Scene Files

No serialization layer exists. Scenes are hardcoded in C++.
- Add a **scene serialization** system (JSON or binary) so levels can be loaded/saved without recompiling.
- This is the first step toward building a **level editor**.

### 11. Input System Upgrade

- Add **input action mapping** (e.g., "Jump" → Spacebar, "Shoot" → LMB) so controls are remappable.
- Add gamepad support via SDL_GameController.

### 12. Memory Management

- Replace scattered `std::shared_ptr` usage with a **custom allocator** or **arena allocator** for frame-temporary data.
- Pool allocations for frequently created/destroyed objects (bullets, particles, effects).

---

## Suggested Priority Order

| Phase | Focus | Why |
|-------|-------|-----|
| **1** | ECS sparse-set + entity recycling | Every system touches ECS; fixing it first compounds |
| **2** | Material System + Draw Command Queue | Decouples rendering from raw GL, enables batching |
| **3** | Model Loading (glTF/Assimp) | Without it, you can't build real scenes |
| **4** | Dynamic Lighting (UBO-based) | Essential for FPS games, currently hardcoded |
| **5** | Fixed timestep loop | Correctness for physics & gameplay |
| **6** | HDR + Bloom post-processing | Biggest visual bang for the buck |
| **7** | RHI abstraction | Clean DX12 support, future Vulkan path |
| **8** | Scene serialization | Enables editor workflows |
| **9** | System scheduler + profiling | Scalability and debugging |

> [!TIP]
> The single biggest architectural win is **ECS + Render Command Queue** together. Once entities produce render commands instead of immediately calling GL, you can sort, batch, instance, and parallelize rendering trivially. Every feature after that (deferred shading, LOD, frustum culling, occlusion culling) becomes a matter of adding filters to the command queue.
