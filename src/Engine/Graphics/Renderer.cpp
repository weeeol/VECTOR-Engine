#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_ttf.h>

#include <vector>

namespace VECTOR {

    Renderer::Renderer() : m_Window(nullptr), m_GLContext(nullptr) {}

    Renderer::~Renderer() {
        Shutdown();
    }

    bool Renderer::Initialize(const std::string& title, int width, int height) {
        m_Width = width;
        m_Height = height;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        m_Window = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
        );

        if (!m_Window) {
            VECTOR_LOG_ERROR("Failed to create SDL window for OpenGL.");
            return false;
        }

        m_GLContext = SDL_GL_CreateContext(m_Window);
        if (!m_GLContext) {
            VECTOR_LOG_ERROR("Failed to create OpenGL context.");
            return false;
        }

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            VECTOR_LOG_ERROR("Failed to initialize GLEW.");
            return false;
        }

        if (TTF_Init() == -1) {
            VECTOR_LOG_ERROR("Failed to initialize SDL_ttf.");
            return false;
        }

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        m_DefaultShader = ResourceManager::Get().LoadShader("Default3D", "assets/engine/shaders/main3D.vert", "assets/engine/shaders/main3D.frag");
        m_DepthShader = ResourceManager::Get().LoadShader("Depth", "assets/engine/shaders/depth.vert", "assets/engine/shaders/depth.frag");
        m_PostProcessShader = ResourceManager::Get().LoadShader("PostProcess", "assets/engine/shaders/postprocess.vert", "assets/engine/shaders/postprocess.frag");
        m_2DShader = ResourceManager::Get().LoadShader("Default2D", "assets/engine/shaders/main2D.vert", "assets/engine/shaders/main2D.frag");

        SetupFBOs();
        SetupScreenQuad();

        // Setup Quad for 2D rendering
        float vertices[] = {
            // pos      // tex
            0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,

            0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f
        };
        
        glGenVertexArrays(1, &m_QuadVAO);
        glGenBuffers(1, &m_QuadVBO);
        
        glBindVertexArray(m_QuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return true;
    }

    void Renderer::Shutdown() {
        if (m_DepthMapFBO) glDeleteFramebuffers(1, &m_DepthMapFBO);
        if (m_DepthMapTexture) glDeleteTextures(1, &m_DepthMapTexture);
        if (m_PostProcessFBO) glDeleteFramebuffers(1, &m_PostProcessFBO);
        if (m_PostProcessTexture) glDeleteTextures(1, &m_PostProcessTexture);
        if (m_PostProcessRBO) glDeleteRenderbuffers(1, &m_PostProcessRBO);
        if (m_ScreenQuadVAO) glDeleteVertexArrays(1, &m_ScreenQuadVAO);
        if (m_ScreenQuadVBO) glDeleteBuffers(1, &m_ScreenQuadVBO);

        if (m_QuadVAO) glDeleteVertexArrays(1, &m_QuadVAO);
        if (m_QuadVBO) glDeleteBuffers(1, &m_QuadVBO);
        m_DefaultShader.reset();
        m_2DShader.reset();

        for (auto& pair : m_TextTextureCache) {
            glDeleteTextures(1, &pair.second.id);
        }
        m_TextTextureCache.clear();

        TTF_Quit();

        if (m_GLContext) {
            SDL_GL_DeleteContext(m_GLContext);
            m_GLContext = nullptr;
        }
        if (m_Window) {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    void Renderer::Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::Present() {
        SDL_GL_SwapWindow(m_Window);
    }

    void Renderer::SetViewProjection(const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection) {
        m_ViewPos = viewPos;
        m_ViewMatrix = view;
        m_ProjectionMatrix = projection;

        // Calculate light space matrix for shadows
        glm::vec3 sunDir = glm::normalize(glm::vec3(-0.2f, 1.0f, 0.3f));
        glm::vec3 lightPos = sunDir * 80.0f; // Sun direction * distance
        glm::mat4 lightProjection = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 1.0f, 150.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        m_LightSpaceMatrix = lightProjection * lightView;
    }

    void Renderer::SetupFBOs() {
        // 1. Shadow map FBO
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

        // 2. Post process FBO
        glGenFramebuffers(1, &m_PostProcessFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessFBO);
        glGenTextures(1, &m_PostProcessTexture);
        glBindTexture(GL_TEXTURE_2D, m_PostProcessTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PostProcessTexture, 0);
        glGenRenderbuffers(1, &m_PostProcessRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_PostProcessRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_PostProcessRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Renderer::SetupScreenQuad() {
        float quadVertices[] = {
            // positions   // texCoords
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

    void Renderer::BeginShadowPass() {
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, m_DepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT); // Peter panning fix
    }

    void Renderer::BeginMainPass() {
        glCullFace(GL_BACK);
        glViewport(0, 0, m_Width, m_Height);
        glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::EndPostProcessPass() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        m_PostProcessShader->Bind();
        m_PostProcessShader->SetInt("screenTexture", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_PostProcessTexture);

        glBindVertexArray(m_ScreenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
    }

    void Renderer::DrawMesh(const Mesh* mesh, const Shader* shader, const Texture2D* texture, const glm::mat4& model, const glm::vec4& color) {
        const Shader* activeShader = shader ? shader : m_DefaultShader.get();
        activeShader->Bind();

        activeShader->SetMat4("model", model);
        
        if (activeShader == m_DepthShader.get()) {
            activeShader->SetMat4("lightSpaceMatrix", m_LightSpaceMatrix);
        } else {
            activeShader->SetMat4("view", m_ViewMatrix);
            activeShader->SetMat4("projection", m_ProjectionMatrix);
            activeShader->SetMat4("lightSpaceMatrix", m_LightSpaceMatrix);
            
            // Convert vec4 to vec3 for now since our shader uses objectColor vec3
            activeShader->SetVec3("objectColor", glm::vec3(color));
            
            activeShader->SetVec3("lightPos", glm::vec3(0.0f, 10.0f, 10.0f));
            activeShader->SetVec3("lightColor", glm::vec3(1.0f, 0.8f, 0.6f)); // Warm point light
            activeShader->SetVec3("viewPos", m_ViewPos);
            
            glm::vec3 sunDir = glm::normalize(glm::vec3(-0.2f, 1.0f, 0.3f));
            activeShader->SetVec3("sunDir", sunDir);

            activeShader->SetInt("isUnlit", m_UnlitMode ? 1 : 0);

            activeShader->SetInt("shadowMap", 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_DepthMapTexture);
            
            if (texture) {
                texture->Bind(0);
                activeShader->SetInt("diffuseTexture", 0);
                activeShader->SetInt("hasTexture", 1);
            } else {
                activeShader->SetInt("hasTexture", 0);
            }
        }

        if (texture) {
            // Need to bind texture
            texture->Bind(0);
            // Assuming shader has "diffuseTexture" uniform
            // activeShader->SetInt("diffuseTexture", 0);
        }

        if (mesh) {
            mesh->Draw();
        }

        if (texture) {
            texture->Unbind();
        }
        activeShader->Unbind();
    }

    void Renderer::BeginUI() {
        m_2DShader->Bind();
        glDisable(GL_DEPTH_TEST);

        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, -1.0f, 1.0f);
        m_2DShader->SetMat4("projection", projection);
        
        glBindVertexArray(m_QuadVAO);
    }

    void Renderer::DrawUIRect(int x, int y, int w, int h, const glm::vec4& color) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(w, h, 1.0f));
        m_2DShader->SetMat4("model", model);

        m_2DShader->SetVec4("spriteColor", color);
        m_2DShader->SetInt("useTexture", 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void Renderer::DrawUIText(const std::string& text, int x, int y, const glm::vec4& renderColor, int fontSize) {
        if (text.empty()) return;

        // Use cached texture or create it
        // We round colors to int for the cache key to avoid precision issues
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
            SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
            if (!surface) return;

            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            int mode = GL_RGB;
            int format = GL_RGB;
            if (surface->format->BytesPerPixel == 4) {
                mode = GL_RGBA;
                if (surface->format->Rmask == 0x00ff0000) {
                    format = GL_BGRA;
                } else {
                    format = GL_RGBA;
                }
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch / surface->format->BytesPerPixel);
            glTexImage2D(GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); 
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            textTex.id = texture;
            textTex.w = surface->w;
            textTex.h = surface->h;
            
            m_TextTextureCache[cacheKey] = textTex;
            SDL_FreeSurface(surface);
        } else {
            textTex = m_TextTextureCache[cacheKey];
        }

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(textTex.w, textTex.h, 1.0f));
        m_2DShader->SetMat4("model", model);

        // For blended text, spriteColor is usually 1.0f, the color is baked in texture
        m_2DShader->SetVec4("spriteColor", glm::vec4(1.0f));
        m_2DShader->SetInt("useTexture", 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textTex.id);
        m_2DShader->SetInt("text", 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Renderer::EndUI() {
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }

    void Renderer::DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        m_2DShader->Bind();
        glDisable(GL_DEPTH_TEST);

        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, -1.0f, 1.0f);
        m_2DShader->SetMat4("projection", projection);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(w, h, 1.0f));
        m_2DShader->SetMat4("model", model);

        m_2DShader->SetVec4("spriteColor", glm::vec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f));
        m_2DShader->SetInt("useTexture", 0);

        glBindVertexArray(m_QuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
    }

    void Renderer::DrawText(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b, int fontSize) {
        if (text.empty()) return;

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

            SDL_Color color = {r, g, b, 255};
            SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
            if (!surface) return;

            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            int mode = GL_RGB;
            int format = GL_RGB;
            if (surface->format->BytesPerPixel == 4) {
                mode = GL_RGBA;
                if (surface->format->Rmask == 0x00ff0000) {
                    format = GL_BGRA;
                } else {
                    format = GL_RGBA;
                }
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch / surface->format->BytesPerPixel);

            glTexImage2D(GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
            
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // reset
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            textTex.id = texture;
            textTex.w = surface->w;
            textTex.h = surface->h;
            
            m_TextTextureCache[cacheKey] = textTex;
            
            glBindTexture(GL_TEXTURE_2D, 0);
            SDL_FreeSurface(surface);
        } else {
            textTex = m_TextTextureCache[cacheKey];
        }

        m_2DShader->Bind();
        glDisable(GL_DEPTH_TEST);

        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, -1.0f, 1.0f);
        m_2DShader->SetMat4("projection", projection);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(textTex.w, textTex.h, 1.0f));
        m_2DShader->SetMat4("model", model);

        m_2DShader->SetVec4("spriteColor", glm::vec4(1.0f));
        m_2DShader->SetInt("useTexture", 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textTex.id);
        m_2DShader->SetInt("text", 0);

        glBindVertexArray(m_QuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

} // namespace VECTOR
