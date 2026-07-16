#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_ttf.h>
#include <GL/glew.h>

#include <vector>

namespace VECTOR {

    Renderer::Renderer() {}

    Renderer::~Renderer() {
        Shutdown();
    }

    bool Renderer::Initialize(const std::string& title, int width, int height) {
        VECTOR_LOG_INFO("Renderer::Initialize start");
        m_Width = width;
        m_Height = height;

        // Ensure SDL is initialized for input and font rendering (even if using DX12)
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
            VECTOR_LOG_ERROR("Failed to initialize SDL.");
            return false;
        }

        VECTOR_LOG_INFO("Initializing TTF...");
        if (TTF_Init() == -1) {
            VECTOR_LOG_ERROR("Failed to initialize SDL_ttf.");
            return false;
        }

        // Set API BEFORE creating window
#if defined(WIN32)
        // RendererAPI::SetAPI(RendererAPI::API::DirectX12); // Uncomment to use DX12 backend
#endif

        WindowProps props(title, width, height);
        m_Window.reset(Window::Create(props));
        
        m_RendererAPI.reset(RendererAPI::Create());
        m_RendererAPI->Init();

        VECTOR_LOG_INFO("Loading Shaders...");
        m_DefaultShader = ResourceManager::Get().LoadShader("Default3D", "assets/engine/shaders/main3D.vert", "assets/engine/shaders/main3D.frag");
        m_DepthShader = ResourceManager::Get().LoadShader("Depth", "assets/engine/shaders/depth.vert", "assets/engine/shaders/depth.frag");
        m_PostProcessShader = ResourceManager::Get().LoadShader("PostProcess", "assets/engine/shaders/postprocess.vert", "assets/engine/shaders/postprocess.frag");
        m_2DShader = ResourceManager::Get().LoadShader("Default2D", "assets/engine/shaders/main2D.vert", "assets/engine/shaders/main2D.frag");

        VECTOR_LOG_INFO("Setup FBOs...");
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
        
        m_QuadVertexArray.reset(VertexArray::Create());
        std::shared_ptr<VertexBuffer> vbo;
        vbo.reset(VertexBuffer::Create(vertices, sizeof(vertices)));
        vbo->SetLayoutType(BufferLayoutType::Quad2D);
        m_QuadVertexArray->AddVertexBuffer(vbo);

        return true;
    }

    void Renderer::Shutdown() {
        m_ShadowMapFramebuffer.reset();
        m_PostProcessFramebuffer.reset();
        m_ScreenQuadVertexArray.reset();
        m_QuadVertexArray.reset();

        m_DefaultShader.reset();
        m_DepthShader.reset();
        m_PostProcessShader.reset();
        m_2DShader.reset();

        m_TextTextureCache.clear();

        m_RendererAPI.reset();
        m_Window.reset();

        TTF_Quit();
        SDL_Quit();
    }

    void Renderer::Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        m_RendererAPI->SetClearColor(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
        m_RendererAPI->Clear();
    }

    void Renderer::Present() {
        m_Window->OnUpdate();
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
        FramebufferSpecification shadowSpec;
        shadowSpec.Width = SHADOW_WIDTH;
        shadowSpec.Height = SHADOW_HEIGHT;
        shadowSpec.DepthOnly = true;
        m_ShadowMapFramebuffer = Framebuffer::Create(shadowSpec);

        // 2. Post process FBO
        FramebufferSpecification postProcessSpec;
        postProcessSpec.Width = m_Width;
        postProcessSpec.Height = m_Height;
        postProcessSpec.DepthOnly = false;
        m_PostProcessFramebuffer = Framebuffer::Create(postProcessSpec);
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

        m_ScreenQuadVertexArray.reset(VertexArray::Create());
        
        std::shared_ptr<VertexBuffer> vbo;
        vbo.reset(VertexBuffer::Create(quadVertices, sizeof(quadVertices)));
        vbo->SetLayoutType(BufferLayoutType::Quad2D);
        m_ScreenQuadVertexArray->AddVertexBuffer(vbo);
    }

    void Renderer::BeginShadowPass() {
        m_ShadowMapFramebuffer->Bind();
        m_RendererAPI->Clear();
    }

    void Renderer::BeginMainPass() {
        m_PostProcessFramebuffer->Bind();
        m_RendererAPI->Clear();
    }

    void Renderer::EndPostProcessPass() {
        m_PostProcessFramebuffer->Unbind();
        m_RendererAPI->Clear();

        m_PostProcessShader->Bind();
        m_PostProcessShader->SetInt("screenTexture", 0);

        // Bind the post process texture
        // Note: For full abstraction we should have a Texture2D::CreateFromID or similar, 
        // but since texture binding for FBOs is currently manual in OpenGL, we'll leave a TODO here
        // and temporarily retain the OpenGL call for the color attachment binding until a RenderCommand queue is built
        // TODO: Abstract this using Texture2D API
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_PostProcessFramebuffer->GetColorAttachmentRendererID());

        m_ScreenQuadVertexArray->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_ScreenQuadVertexArray->Unbind();
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
            // TODO: Abstract Texture binding
            glBindTexture(GL_TEXTURE_2D, m_ShadowMapFramebuffer->GetDepthAttachmentRendererID());
            
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
        
        m_QuadVertexArray->Bind();
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
        m_QuadVertexArray->Unbind();
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

        m_QuadVertexArray->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_QuadVertexArray->Unbind();

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

        m_QuadVertexArray->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_QuadVertexArray->Unbind();

        glEnable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

} // namespace VECTOR
