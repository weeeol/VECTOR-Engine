#include "Engine/Graphics/OpenGL/OpenGLRenderer.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

namespace VECTOR {

    OpenGLRenderer::OpenGLRenderer() : m_Window(nullptr), m_GLContext(nullptr) {}

    OpenGLRenderer::~OpenGLRenderer() {
        Shutdown();
    }

    bool OpenGLRenderer::Initialize(const std::string& title, int width, int height) {
        VECTOR_LOG_INFO("Renderer::Initialize start");
        m_Width = width;
        m_Height = height;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        VECTOR_LOG_INFO("Creating SDL Window...");
        m_Window = SDL_CreateWindow(
            title.c_str(),
            width,
            height,
            SDL_WINDOW_OPENGL
        );

        if (!m_Window) {
            VECTOR_LOG_ERROR("Failed to create SDL Window");
            return false;
        }

        int pw, ph;
        SDL_GetWindowSizeInPixels(m_Window, &pw, &ph);
        m_Width = pw;
        m_Height = ph;

        VECTOR_LOG_INFO("Creating OpenGL Context...");
        m_GLContext = SDL_GL_CreateContext(m_Window);
        if (!m_GLContext) {
            VECTOR_LOG_ERROR("Failed to create OpenGL context.");
            return false;
        }

        VECTOR_LOG_INFO("Initializing GLEW...");
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            VECTOR_LOG_ERROR("Failed to initialize GLEW.");
            return false;
        }

        VECTOR_LOG_INFO("Initializing TTF...");
        if (!TTF_Init()) {
            VECTOR_LOG_ERROR("Failed to initialize SDL_ttf.");
            return false;
        }

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        VECTOR_LOG_INFO("Loading Shaders...");
        m_DefaultShader = ResourceManager::Get().LoadShader("Default3D", "assets/engine/shaders/main3D.vert", "assets/engine/shaders/main3D.frag");
        m_DepthShader = ResourceManager::Get().LoadShader("Depth", "assets/engine/shaders/depth.vert", "assets/engine/shaders/depth.frag");
        m_PostProcessShader = ResourceManager::Get().LoadShader("PostProcess", "assets/engine/shaders/postprocess.vert", "assets/engine/shaders/postprocess.frag");
        m_BloomDownsampleShader = ResourceManager::Get().LoadShader("BloomDown", "assets/engine/shaders/postprocess.vert", "assets/engine/shaders/bloom_downsample.frag");
        m_BloomUpsampleShader = ResourceManager::Get().LoadShader("BloomUp", "assets/engine/shaders/postprocess.vert", "assets/engine/shaders/bloom_upsample.frag");
        m_UIShader = ResourceManager::Get().LoadShader("Default2D", "assets/engine/shaders/main2D.vert", "assets/engine/shaders/main2D.frag");
        m_GBufferShader = ResourceManager::Get().LoadShader("GBuffer", "assets/engine/shaders/gbuffer.vert", "assets/engine/shaders/gbuffer.frag");
        m_DeferredLightShader = ResourceManager::Get().LoadShader("DeferredLight", "assets/engine/shaders/deferred_light.vert", "assets/engine/shaders/deferred_light.frag");

        VECTOR_LOG_INFO("Setting up UBO...");
        m_PerFrameUBO = UniformBuffer::Create(static_cast<uint32_t>(sizeof(PerFrameData)), 0);

        UniformBuffer::BindShaderBlock(m_DefaultShader->GetID(), "PerFrameData", 0);
        UniformBuffer::BindShaderBlock(m_DepthShader->GetID(), "PerFrameData", 0);
        UniformBuffer::BindShaderBlock(m_GBufferShader->GetID(), "PerFrameData", 0);
        UniformBuffer::BindShaderBlock(m_DeferredLightShader->GetID(), "PerFrameData", 0);

        VECTOR_LOG_INFO("Setting up Light UBO...");
        m_LightUBO = UniformBuffer::Create(static_cast<uint32_t>(sizeof(LightUBOData)), 1);
        UniformBuffer::BindShaderBlock(m_DefaultShader->GetID(), "LightDataBlock", 1);
        UniformBuffer::BindShaderBlock(m_DeferredLightShader->GetID(), "LightDataBlock", 1);

        m_LightData.numPointLights = 0;
        m_LightData.dirLight.colorAndIntensity = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        m_LightData.dirLight.direction = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);

        VECTOR_LOG_INFO("Creating default materials...");
        m_DefaultMaterial = std::make_shared<Material>();
        m_DefaultMaterial->shader = m_DefaultShader;
        m_DefaultMaterial->albedoColor = glm::vec4(1.0f);

        m_DepthMaterial = std::make_shared<Material>();
        m_DepthMaterial->shader = m_DepthShader;
        m_DepthMaterial->isUnlit = true;

        VECTOR_LOG_INFO("Setup FBOs...");
        SetupDepthFBO();
        SetupPostProcessFBO();
        SetupBloomFBOs();
        SetupGBufferFBO();
        SetupScreenQuad();

        float vertices[] = {
            0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,

            0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f
        };
        
        glGenVertexArrays(1, &m_UIQuadVAO);
        glGenBuffers(1, &m_UIQuadVBO);
        
        glBindVertexArray(m_UIQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_UIQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.105f, 0.11f, 1.0f);

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForOpenGL(m_Window, m_GLContext);
        ImGui_ImplOpenGL3_Init("#version 330 core");

        VECTOR_LOG_INFO("Renderer initialized successfully.");
        return true;
    }

    void OpenGLRenderer::Shutdown() {
        // Cleanup Dear ImGui
        if (ImGui::GetCurrentContext()) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
        }

        m_LightUBO.reset();
        m_PerFrameUBO.reset();

        for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
            if (m_BloomMips[i].fbo) glDeleteFramebuffers(1, &m_BloomMips[i].fbo);
            if (m_BloomMips[i].texture) glDeleteTextures(1, &m_BloomMips[i].texture);
        }
        m_DefaultMaterial.reset();
        m_DepthMaterial.reset();

        if (m_DepthMapFBO) glDeleteFramebuffers(1, &m_DepthMapFBO);
        if (m_DepthMapTexture) glDeleteTextures(1, &m_DepthMapTexture);
        if (m_PostProcessFBO) glDeleteFramebuffers(1, &m_PostProcessFBO);
        if (m_PostProcessTexture) glDeleteTextures(1, &m_PostProcessTexture);
        if (m_PostProcessRBO) glDeleteRenderbuffers(1, &m_PostProcessRBO);
        if (m_ScreenQuadVAO) glDeleteVertexArrays(1, &m_ScreenQuadVAO);
        if (m_ScreenQuadVBO) glDeleteBuffers(1, &m_ScreenQuadVBO);

        if (m_GBufferFBO) glDeleteFramebuffers(1, &m_GBufferFBO);
        if (m_GPosition) glDeleteTextures(1, &m_GPosition);
        if (m_GNormal) glDeleteTextures(1, &m_GNormal);
        if (m_GAlbedo) glDeleteTextures(1, &m_GAlbedo);
        if (m_GMetallicRoughnessAO) glDeleteTextures(1, &m_GMetallicRoughnessAO);
        if (m_GBufferRBO) glDeleteRenderbuffers(1, &m_GBufferRBO);

        if (m_UIQuadVAO) glDeleteVertexArrays(1, &m_UIQuadVAO);
        if (m_UIQuadVBO) glDeleteBuffers(1, &m_UIQuadVBO);
        m_DefaultShader.reset();
        m_DepthShader.reset();
        m_PostProcessShader.reset();
        m_UIShader.reset();
        m_GBufferShader.reset();
        m_DeferredLightShader.reset();

        for (auto& pair : m_TextTextureCache) {
            glDeleteTextures(1, &pair.second.id);
        }
        m_TextTextureCache.clear();

        TTF_Quit();

        if (m_GLContext) {
            SDL_GL_DestroyContext(m_GLContext);
            m_GLContext = nullptr;
        }
        if (m_Window) {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    void OpenGLRenderer::Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void OpenGLRenderer::Present() {
        SDL_GL_SwapWindow(m_Window);
    }

    void OpenGLRenderer::SetResolution(int width, int height) {
        if (!m_Window) return;
        SDL_SetWindowSize(m_Window, width, height);
        SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        
        int pw, ph;
        SDL_GetWindowSizeInPixels(m_Window, &pw, &ph);
        m_Width = pw;
        m_Height = ph;
        glViewport(0, 0, m_Width, m_Height);
        RecreateScreenFBOs();
    }

    void OpenGLRenderer::SetFullscreen(bool fullscreen, bool borderless) {
        if (!m_Window) return;
        if (fullscreen) {
            if (borderless) {
                SDL_SetWindowFullscreenMode(m_Window, nullptr);
                SDL_SetWindowFullscreen(m_Window, true);
            } else {
                SDL_SetWindowFullscreenMode(m_Window, nullptr); // Or specific mode if needed
                SDL_SetWindowFullscreen(m_Window, true);
            }
        } else {
            SDL_SetWindowFullscreen(m_Window, false);
        }

        int pw, ph;
        SDL_GetWindowSizeInPixels(m_Window, &pw, &ph);
        m_Width = pw;
        m_Height = ph;
        glViewport(0, 0, m_Width, m_Height);
        RecreateScreenFBOs();
    }

    void OpenGLRenderer::SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) {
        m_ViewPos = viewPos;
        m_ViewMatrix = view;
        m_ProjectionMatrix = projection;

        glm::vec3 sunDir = glm::normalize(glm::vec3(-0.2f, 1.0f, 0.3f));
        glm::vec3 lightPos = sunDir * 80.0f;
        glm::mat4 lightProjection = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 1.0f, 150.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        m_LightSpaceMatrix = lightProjection * lightView;

        m_PerFrameData.view = view;
        m_PerFrameData.projection = projection;
        m_PerFrameData.lightSpaceMatrix = m_LightSpaceMatrix;
        m_PerFrameData.viewPos = glm::vec4(viewPos, 1.0f);
        m_PerFrameData.sunDir = glm::vec4(sunDir, 0.0f);
        m_PerFrameData.sunColor = glm::vec4(0.9f, 0.9f, 0.8f, 1.0f);
        m_PerFrameData.lightPos = glm::vec4(0.0f, 10.0f, 10.0f, 1.0f);
        m_PerFrameData.lightColor = glm::vec4(1.0f, 0.8f, 0.6f, 1.0f);

        m_PerFrameUBO->SetData(&m_PerFrameData, sizeof(PerFrameData));

        m_CameraFrustum = CreateFrustumFromMatrix(projection * view);
    }

    void OpenGLRenderer::SetupDepthFBO() {
        glGenFramebuffers(1, &m_DepthMapFBO);
        glGenTextures(1, &m_DepthMapTexture);
        glBindTexture(GL_TEXTURE_2D, m_DepthMapTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindFramebuffer(GL_FRAMEBUFFER, m_DepthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthMapTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderer::SetupPostProcessFBO() {
        glGenFramebuffers(1, &m_PostProcessFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessFBO);
        glGenTextures(1, &m_PostProcessTexture);
        glBindTexture(GL_TEXTURE_2D, m_PostProcessTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PostProcessTexture, 0);
        glGenRenderbuffers(1, &m_PostProcessRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_PostProcessRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_PostProcessRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderer::SetupGBufferFBO() {
        if (m_GBufferFBO) {
            glDeleteFramebuffers(1, &m_GBufferFBO);
            glDeleteTextures(1, &m_GPosition);
            glDeleteTextures(1, &m_GNormal);
            glDeleteTextures(1, &m_GAlbedo);
            glDeleteTextures(1, &m_GMetallicRoughnessAO);
            glDeleteRenderbuffers(1, &m_GBufferRBO);
        }

        glGenFramebuffers(1, &m_GBufferFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_GBufferFBO);

        // 1. Position color buffer
        glGenTextures(1, &m_GPosition);
        glBindTexture(GL_TEXTURE_2D, m_GPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_GPosition, 0);

        // 2. Normal color buffer
        glGenTextures(1, &m_GNormal);
        glBindTexture(GL_TEXTURE_2D, m_GNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_GNormal, 0);

        // 3. Albedo + Specular/Unlit color buffer (RGB: albedo, A: isUnlit)
        glGenTextures(1, &m_GAlbedo);
        glBindTexture(GL_TEXTURE_2D, m_GAlbedo);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_GAlbedo, 0);

        // 4. Metallic + Roughness + AO color buffer (R: metallic, G: roughness, B: AO)
        glGenTextures(1, &m_GMetallicRoughnessAO);
        glBindTexture(GL_TEXTURE_2D, m_GMetallicRoughnessAO);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_GMetallicRoughnessAO, 0);

        // Color attachments list
        unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, attachments);

        // Depth buffer
        glGenRenderbuffers(1, &m_GBufferRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_GBufferRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_GBufferRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            VECTOR_LOG_ERROR("GBuffer Framebuffer not complete!");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderer::RecreateScreenFBOs() {
        if (m_PostProcessFBO) {
            glDeleteFramebuffers(1, &m_PostProcessFBO);
            glDeleteTextures(1, &m_PostProcessTexture);
            glDeleteRenderbuffers(1, &m_PostProcessRBO);
        }
        for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
            if (m_BloomMips[i].fbo) {
                glDeleteFramebuffers(1, &m_BloomMips[i].fbo);
                glDeleteTextures(1, &m_BloomMips[i].texture);
            }
        }
        SetupPostProcessFBO();
        SetupBloomFBOs();
        SetupGBufferFBO();
    }

    void OpenGLRenderer::SetupBloomFBOs() {
        int mipWidth = m_Width / 2;
        int mipHeight = m_Height / 2;

        for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
            m_BloomMips[i].width = mipWidth;
            m_BloomMips[i].height = mipHeight;

            glGenFramebuffers(1, &m_BloomMips[i].fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, m_BloomMips[i].fbo);

            glGenTextures(1, &m_BloomMips[i].texture);
            glBindTexture(GL_TEXTURE_2D, m_BloomMips[i].texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mipWidth, mipHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BloomMips[i].texture, 0);

            mipWidth /= 2;
            mipHeight /= 2;
            if (mipWidth < 1) mipWidth = 1;
            if (mipHeight < 1) mipHeight = 1;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderer::RenderBloomPasses() {
        glDisable(GL_DEPTH_TEST);

        m_BloomDownsampleShader->Bind();
        m_BloomDownsampleShader->SetInt("srcTexture", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_PostProcessTexture);
        m_BloomDownsampleShader->SetVec2("srcResolution", glm::vec2((float)m_Width, (float)m_Height));
        m_BloomDownsampleShader->SetFloat("threshold", m_BloomThreshold);

        glBindFramebuffer(GL_FRAMEBUFFER, m_BloomMips[0].fbo);
        glViewport(0, 0, m_BloomMips[0].width, m_BloomMips[0].height);
        glBindVertexArray(m_ScreenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        for (int i = 1; i < BLOOM_MIP_COUNT; i++) {
            glBindTexture(GL_TEXTURE_2D, m_BloomMips[i - 1].texture);
            m_BloomDownsampleShader->SetVec2("srcResolution", glm::vec2((float)m_BloomMips[i - 1].width, (float)m_BloomMips[i - 1].height));
            m_BloomDownsampleShader->SetFloat("threshold", 0.0f);

            glBindFramebuffer(GL_FRAMEBUFFER, m_BloomMips[i].fbo);
            glViewport(0, 0, m_BloomMips[i].width, m_BloomMips[i].height);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        m_BloomUpsampleShader->Bind();
        m_BloomUpsampleShader->SetInt("srcTexture", 0);
        m_BloomUpsampleShader->SetFloat("filterRadius", m_BloomFilterRadius);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE); 
        glBlendEquation(GL_FUNC_ADD);

        for (int i = BLOOM_MIP_COUNT - 1; i > 0; i--) {
            glBindTexture(GL_TEXTURE_2D, m_BloomMips[i].texture);

            glBindFramebuffer(GL_FRAMEBUFFER, m_BloomMips[i - 1].fbo);
            glViewport(0, 0, m_BloomMips[i - 1].width, m_BloomMips[i - 1].height);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
    }

    void OpenGLRenderer::SetupScreenQuad() {
        float quadVertices[] = {
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };
        glGenVertexArrays(1, &m_ScreenQuadVAO);
        glGenBuffers(1, &m_ScreenQuadVBO);
        glBindVertexArray(m_ScreenQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_ScreenQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }

    void OpenGLRenderer::SubmitMesh(const Mesh* mesh, const Material* material, const glm::mat4& model) {
        if (!mesh) return;

        const AABB& localAABB = mesh->GetAABB();
        
        glm::vec3 globalCenter = glm::vec3(model * glm::vec4(localAABB.center, 1.0f));
        
        glm::vec3 right = glm::vec3(model[0]);
        glm::vec3 up = glm::vec3(model[1]);
        glm::vec3 forward = glm::vec3(model[2]);
        
        float scaleX = glm::length(right);
        float scaleY = glm::length(up);
        float scaleZ = glm::length(forward);
        glm::vec3 globalExtents = localAABB.extents * glm::vec3(scaleX, scaleY, scaleZ);

        AABB globalAABB(globalCenter, globalExtents);

        if (!IsOnFrustum(m_CameraFrustum, globalAABB)) {
            return; 
        }

        const Material* mat = material ? material : m_DefaultMaterial.get();

        RenderCommand cmd;
        cmd.sortKey = mat->GetSortKey();
        cmd.mesh = mesh;
        cmd.material = mat;
        cmd.modelMatrix = model;
        m_MainQueue.Submit(cmd);

        RenderCommand shadowCmd;
        shadowCmd.sortKey = 0; 
        shadowCmd.mesh = mesh;
        shadowCmd.material = m_DepthMaterial.get();
        shadowCmd.modelMatrix = model;
        m_ShadowQueue.Submit(shadowCmd);
    }

    void OpenGLRenderer::SubmitPointLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) {
        if (m_LightData.numPointLights < MAX_POINT_LIGHTS) {
            int idx = m_LightData.numPointLights++;
            m_LightData.pointLights[idx].positionAndRadius = glm::vec4(position, radius);
            m_LightData.pointLights[idx].colorAndIntensity = glm::vec4(color, intensity);
        }
    }

    void OpenGLRenderer::SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity) {
        m_LightData.dirLight.direction = glm::vec4(glm::normalize(direction), 0.0f);
        m_LightData.dirLight.colorAndIntensity = glm::vec4(color, intensity);
    }

    void OpenGLRenderer::BeginShadowPass() {
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, m_DepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT); 
        m_LastFrameDrawCalls = 0;
    }

    void OpenGLRenderer::FlushShadowPass() {
        m_ShadowQueue.SortAndFlush();
        m_LastFrameDrawCalls += m_ShadowQueue.GetLastDrawCallCount();
    }

    void OpenGLRenderer::BeginMainPass() {
        glDisable(GL_BLEND);
        glCullFace(GL_BACK);
        glViewport(0, 0, m_Width, m_Height);
        glBindFramebuffer(GL_FRAMEBUFFER, m_GBufferFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_DEPTH_TEST);
    }

    void OpenGLRenderer::FlushMainPass() {
        m_LightUBO->SetData(&m_LightData, sizeof(LightUBOData));
        
        m_LightData.numPointLights = 0;

        m_PerFrameUBO->Bind();
        m_LightUBO->Bind();

        // 1. Geometry Pass: Render to G-Buffer
        glEnable(GL_DEPTH_TEST);
        m_MainQueue.SortAndFlushGBuffer(m_GBufferShader.get());

        // 2. Deferred Lighting Pass: Render fullscreen quad with G-Buffer to PostProcess HDR buffer
        glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        if (m_WireframeEnabled) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        m_DeferredLightShader->Bind();

        // Bind G-buffer textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_GPosition);
        m_DeferredLightShader->SetInt("gPosition", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_GNormal);
        m_DeferredLightShader->SetInt("gNormal", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_GAlbedo);
        m_DeferredLightShader->SetInt("gAlbedo", 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, m_GMetallicRoughnessAO);
        m_DeferredLightShader->SetInt("gMetallicRoughnessAO", 3);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, m_DepthMapTexture);
        m_DeferredLightShader->SetInt("shadowMap", 4);

        // Render full screen quad
        glBindVertexArray(m_ScreenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        if (m_WireframeEnabled) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        glEnable(GL_DEPTH_TEST);

        m_LastFrameDrawCalls += m_MainQueue.GetLastDrawCallCount();
    }

    void OpenGLRenderer::EndPostProcessPass() {
        RenderBloomPasses();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_Width, m_Height); 
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        if (m_WireframeEnabled) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        m_PostProcessShader->Bind();
        m_PostProcessShader->SetInt("screenTexture", 0);
        m_PostProcessShader->SetInt("bloomTexture", 1);
        m_PostProcessShader->SetFloat("exposure", m_Exposure);
        m_PostProcessShader->SetFloat("bloomStrength", m_BloomStrength);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_PostProcessTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_BloomMips[0].texture);

        glBindVertexArray(m_ScreenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        if (m_WireframeEnabled) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        glEnable(GL_DEPTH_TEST);
    }

    void OpenGLRenderer::BeginUI() {
        m_UIShader->Bind();
        glDisable(GL_DEPTH_TEST);

        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, -1.0f, 1.0f);
        m_UIShader->SetMat4("projection", projection);
        
        glBindVertexArray(m_UIQuadVAO);
    }

    void OpenGLRenderer::DrawUIRect(int x, int y, int w, int h, const glm::vec4& color) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(w, h, 1.0f));
        m_UIShader->SetMat4("model", model);

        m_UIShader->SetVec4("spriteColor", color);
        m_UIShader->SetInt("useTexture", 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void OpenGLRenderer::DrawUIText(const std::string& text, int x, int y, const glm::vec4& renderColor, int fontSize) {
        if (text.empty()) return;

        int r = (int)(renderColor.r * 255);
        int g = (int)(renderColor.g * 255);
        int b = (int)(renderColor.b * 255);

        std::string cacheKey = text + "_" + std::to_string(r) + "_" + std::to_string(g) + "_" + std::to_string(b) + "_" + std::to_string(fontSize);
        
        TextTexture textTex;
        if (m_TextTextureCache.find(cacheKey) == m_TextTextureCache.end()) {
            if (m_TextTextureCache.size() > 128) {
                for (auto& pair : m_TextTextureCache) {
                    glDeleteTextures(1, &pair.second.id);
                }
                m_TextTextureCache.clear();
            }
            
            TTF_Font* font = ResourceManager::Get().GetFont("assets/font.ttf", fontSize);
            if (!font) return;

            SDL_Color color = {(uint8_t)r, (uint8_t)g, (uint8_t)b, 255};
            SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, color);
            if (!surface) return;

            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            int mode = GL_RGB;
            int format = GL_RGB;
            const SDL_PixelFormatDetails* fmt_details = SDL_GetPixelFormatDetails(surface->format);
            if (fmt_details->bytes_per_pixel == 4) {
                mode = GL_RGBA;
                if (fmt_details->Rmask == 0x00ff0000) {
                    format = GL_BGRA;
                } else {
                    format = GL_RGBA;
                }
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch / fmt_details->bytes_per_pixel);
            glTexImage2D(GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); 
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            textTex.id = texture;
            textTex.w = surface->w;
            textTex.h = surface->h;
            
            m_TextTextureCache[cacheKey] = textTex;
            SDL_DestroySurface(surface);
        } else {
            textTex = m_TextTextureCache[cacheKey];
        }

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(textTex.w, textTex.h, 1.0f));
        m_UIShader->SetMat4("model", model);

        m_UIShader->SetVec4("spriteColor", glm::vec4(1.0f));
        m_UIShader->SetInt("useTexture", 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textTex.id);
        m_UIShader->SetInt("text", 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void OpenGLRenderer::EndUI() {
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }

    void OpenGLRenderer::BeginImGuiFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void OpenGLRenderer::EndImGuiFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void OpenGLRenderer::SetWireframeMode(bool enabled) {
        m_WireframeEnabled = enabled;
        if (enabled) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    std::string OpenGLRenderer::GetRendererInfo() const {
        const GLubyte* rendererStr = glGetString(GL_RENDERER);
        if (rendererStr) {
            return std::string(reinterpret_cast<const char*>(rendererStr));
        }
        return "Unknown OpenGL Renderer";
    }

} // namespace VECTOR
