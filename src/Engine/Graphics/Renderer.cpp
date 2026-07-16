#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_ttf.h>
#include <GL/glew.h>

#include <vector>

#ifdef VECTOR_BUILD_DIRECTX
#include "Engine/Graphics/DirectX/DirectX12Context.hpp"
#include "Engine/Graphics/DirectX/DirectX12Shader.hpp"
#include "Engine/Graphics/DirectX/DirectX12Texture2D.hpp"
#include "Engine/Graphics/DirectX/DirectX12Framebuffer.hpp"

#ifdef DrawText
#undef DrawText
#endif
#endif

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
        VECTOR_LOG_INFO("Setup FBOs done. Setting up Screen Quad...");
        SetupScreenQuad();
        VECTOR_LOG_INFO("Screen Quad done. Setting up UI Quad...");

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
        VECTOR_LOG_INFO("UI Quad done. Renderer initialized successfully.");

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

#ifdef VECTOR_BUILD_DIRECTX
        auto context = DirectX12Context::Get();
        if (context) {
            auto cmdList = context->GetCommandList();
            float clearColor[] = { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };

            if (m_ActiveFBO) {
                auto dxFBO = dynamic_cast<DirectX12Framebuffer*>(m_ActiveFBO);
                if (dxFBO) {
                    D3D12_CPU_DESCRIPTOR_HANDLE rtv = dxFBO->GetRTV();
                    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
                    
                    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dxFBO->GetDSV();
                    cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
                }
            } else {
                auto rtvHandle = context->GetCurrentRTV();
                auto backBuffer = context->GetCurrentBackBuffer();
                
                // Transition to render target
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = backBuffer;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                cmdList->ResourceBarrier(1, &barrier);
                
                cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
                cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
                
                D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, 1.0f };
                D3D12_RECT scissor = { 0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height) };
                cmdList->RSSetViewports(1, &viewport);
                cmdList->RSSetScissorRects(1, &scissor);
            }
            
            // Set Root Signature and Descriptor Heaps
            ID3D12DescriptorHeap* descriptorHeaps[] = { context->GetSrvHeap() };
            cmdList->SetDescriptorHeaps(1, descriptorHeaps);
            cmdList->SetGraphicsRootSignature(context->GetRootSignature());
        }
#endif
    }

    void Renderer::Present() {
#ifdef VECTOR_BUILD_DIRECTX
        auto context = DirectX12Context::Get();
        if (context) {
            auto cmdList = context->GetCommandList();
            auto backBuffer = context->GetCurrentBackBuffer();
            
            // Transition back to present
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = backBuffer;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(1, &barrier);
            
            // Close command list and execute
            cmdList->Close();
            ID3D12CommandList* ppCommandLists[] = { cmdList };
            context->GetCommandQueue()->ExecuteCommandLists(1, ppCommandLists);
        }
#endif
        m_Window->OnUpdate(); // Swaps buffers
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
        m_ActiveFBO = m_ShadowMapFramebuffer.get();
        if (m_ActiveFBO) m_ActiveFBO->Bind();
#ifndef VECTOR_BUILD_DIRECTX
        m_RendererAPI->Clear();
#endif
    }

    void Renderer::BeginMainPass() {
        m_ActiveFBO = m_PostProcessFramebuffer.get();
        if (m_ActiveFBO) m_ActiveFBO->Bind();
#ifndef VECTOR_BUILD_DIRECTX
        m_RendererAPI->Clear();
#endif
    }

    void Renderer::EndPostProcessPass() {
        if (m_ActiveFBO) m_ActiveFBO->Unbind();
        m_ActiveFBO = nullptr;

#ifndef VECTOR_BUILD_DIRECTX
        m_RendererAPI->Clear();
#else
        Clear(0, 0, 0, 255); // Clears the backbuffer and transitions it
#endif

        m_PostProcessShader->Bind();
        m_PostProcessShader->SetInt("screenTexture", 0);

#ifdef VECTOR_BUILD_DIRECTX
        auto context = DirectX12Context::Get();
        if (context) {
            auto cmdList = context->GetCommandList();
            auto dxShader = dynamic_cast<DirectX12Shader*>(m_PostProcessShader.get());
            if (dxShader && dxShader->GetPipelineState()) {
                cmdList->SetPipelineState(dxShader->GetPipelineState());
            }

            // Dummy CBV to satisfy Root Signature
            struct DummyCBV { glm::mat4 dummy; } dummyData = {};
            D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = context->UploadConstantBuffer(&dummyData, sizeof(dummyData));
            cmdList->SetGraphicsRootConstantBufferView(0, cbvAddress);

            auto dxFBO = dynamic_cast<DirectX12Framebuffer*>(m_PostProcessFramebuffer.get());
            if (dxFBO) {
                cmdList->SetGraphicsRootDescriptorTable(1, dxFBO->GetColorAttachmentSRV());
            } else {
                cmdList->SetGraphicsRootDescriptorTable(1, context->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
            }

            m_ScreenQuadVertexArray->Bind();

            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmdList->DrawInstanced(6, 1, 0, 0);
        }
        return;
#endif

        // Bind the post process texture
        // Note: For full abstraction we should have a Texture2D::CreateFromID or similar, 
        // but since texture binding for FBOs is currently manual in OpenGL, we'll leave a TODO here
        // and temporarily retain the OpenGL call for the color attachment binding until a RenderCommand queue is built
        // TODO: Abstract this using Texture2D API
#ifndef VECTOR_BUILD_DIRECTX
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_PostProcessFramebuffer->GetColorAttachmentRendererID());

        m_ScreenQuadVertexArray->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_ScreenQuadVertexArray->Unbind();
#endif
    }

    void Renderer::DrawMesh(const Mesh* mesh, const Shader* shader, const Texture2D* texture, const glm::mat4& model, const glm::vec4& color) {
        const Shader* activeShader = shader ? shader : m_DefaultShader.get();
        activeShader->Bind();

#ifdef VECTOR_BUILD_DIRECTX
        auto context = DirectX12Context::Get();
        if (context && mesh) {
            auto cmdList = context->GetCommandList();
            
            auto dxShader = dynamic_cast<const DirectX12Shader*>(activeShader);
            if (dxShader && dxShader->GetPipelineState()) {
                cmdList->SetPipelineState(dxShader->GetPipelineState());
            }

            // Define the CBV struct layout exactly matching the HLSL
            struct ConstantBufferData {
                glm::mat4 model;
                glm::mat4 view;
                glm::mat4 projection;
                glm::mat4 lightSpaceMatrix;
                glm::vec3 objectColor;
                float padding1;
                glm::vec3 lightPos;
                float padding2;
                glm::vec3 lightColor;
                float padding3;
                glm::vec3 viewPos;
                float padding4;
                glm::vec3 sunDir;
                float padding5;
                int isUnlit;
                int hasTexture;
                int hasShadowMap;
                int padding6;
            } cbvData;

            // Transpose matrices because HLSL expects column-major by default
            cbvData.model = glm::transpose(model);
            cbvData.view = glm::transpose(m_ViewMatrix);
            cbvData.projection = glm::transpose(m_ProjectionMatrix);
            cbvData.lightSpaceMatrix = glm::transpose(m_LightSpaceMatrix);
            cbvData.objectColor = glm::vec3(color);
            cbvData.lightPos = glm::vec3(0.0f, 10.0f, 10.0f);
            cbvData.lightColor = glm::vec3(1.0f, 0.8f, 0.6f);
            cbvData.viewPos = m_ViewPos;
            cbvData.sunDir = glm::normalize(glm::vec3(-0.2f, 1.0f, 0.3f));
            cbvData.isUnlit = m_UnlitMode ? 1 : 0;
            cbvData.hasTexture = texture ? 1 : 0;
            cbvData.hasShadowMap = 0;

            D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = context->UploadConstantBuffer(&cbvData, sizeof(cbvData));
            cmdList->SetGraphicsRootConstantBufferView(0, cbvAddress);

            auto dxTexture = dynamic_cast<const DirectX12Texture2D*>(texture);
            if (dxTexture) {
                cmdList->SetGraphicsRootDescriptorTable(1, dxTexture->GetGpuDescriptorHandle());
            } else {
                cmdList->SetGraphicsRootDescriptorTable(1, context->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
            }

            // Assume mesh VertexArray is bound and buffers are set in DX12 VertexArray
            mesh->Draw();
        }
        return;
#endif

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
            texture->Bind(0);
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
#ifndef VECTOR_BUILD_DIRECTX
        glDisable(GL_DEPTH_TEST);
#endif

#ifdef VECTOR_BUILD_DIRECTX
        auto context = DirectX12Context::Get();
        if (context) {
            ID3D12DescriptorHeap* descriptorHeaps[] = { context->GetSrvHeap() };
            context->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
        }
#endif

        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, -1.0f, 1.0f);
        m_2DShader->SetMat4("projection", projection);
        
        m_QuadVertexArray->Bind();
    }

    void Renderer::DrawUIRect(int x, int y, int w, int h, const glm::vec4& color) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(w, h, 1.0f));
        
#ifdef VECTOR_BUILD_DIRECTX
        auto context = DirectX12Context::Get();
        if (context) {
            auto cmdList = context->GetCommandList();
            auto dxShader = dynamic_cast<const DirectX12Shader*>(m_2DShader.get());
            if (dxShader && dxShader->GetPipelineState()) {
                cmdList->SetPipelineState(dxShader->GetPipelineState());
            }

            struct CBV2D {
                glm::mat4 model;
                glm::mat4 projection;
                glm::vec4 spriteColor;
                int useTexture;
                int padding[3];
            } cbvData;
            
            glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, -1.0f, 1.0f);
            cbvData.projection = glm::transpose(projection);
            cbvData.model = glm::transpose(model);
            cbvData.spriteColor = color;
            cbvData.useTexture = 0;

            D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = context->UploadConstantBuffer(&cbvData, sizeof(cbvData));
            cmdList->SetGraphicsRootConstantBufferView(0, cbvAddress);
            cmdList->SetGraphicsRootDescriptorTable(1, context->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmdList->DrawInstanced(6, 1, 0, 0);
        }
        return;
#endif

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
#ifdef VECTOR_BUILD_DIRECTX
                auto context = DirectX12Context::Get();
                if (context) context->FlushCommandQueue();
#endif
                // Shared pointers will automatically clean up when removed from cache!
                m_TextTextureCache.clear();
            }
            
            TTF_Font* font = ResourceManager::Get().GetFont("assets/font.ttf", fontSize);
            if (!font) return;

            SDL_Color color = {(uint8_t)r, (uint8_t)g, (uint8_t)b, 255};
            SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
            if (!surface) return;

            // Convert to a consistent RGBA format for all APIs
            SDL_Surface* formattedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
            SDL_FreeSurface(surface);
            if (!formattedSurface) return;

            textTex.texture = Texture2D::CreateFromPixels(formattedSurface->pixels, formattedSurface->w, formattedSurface->h);
            textTex.w = formattedSurface->w;
            textTex.h = formattedSurface->h;
            
            m_TextTextureCache[cacheKey] = textTex;
            SDL_FreeSurface(formattedSurface);
        } else {
            textTex = m_TextTextureCache[cacheKey];
        }

        if (!textTex.texture) return;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(textTex.w, textTex.h, 1.0f));

#ifdef VECTOR_BUILD_DIRECTX
        auto context = DirectX12Context::Get();
        if (context) {
            auto cmdList = context->GetCommandList();
            auto dxShader = dynamic_cast<const DirectX12Shader*>(m_2DShader.get());
            if (dxShader && dxShader->GetPipelineState()) {
                cmdList->SetPipelineState(dxShader->GetPipelineState());
                cmdList->SetGraphicsRootSignature(context->GetRootSignature());
            }

            struct CBV2D {
                glm::mat4 model;
                glm::mat4 projection;
                glm::vec4 spriteColor;
                int useTexture;
                int padding[3];
            } cbvData;
            
            glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, -1.0f, 1.0f);
            cbvData.model = glm::transpose(model);
            cbvData.projection = glm::transpose(projection);
            cbvData.spriteColor = glm::vec4(1.0f);
            cbvData.useTexture = 1;

            D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = context->UploadConstantBuffer(&cbvData, sizeof(cbvData));
            cmdList->SetGraphicsRootConstantBufferView(0, cbvAddress);

            auto dxTexture = dynamic_cast<const DirectX12Texture2D*>(textTex.texture.get());
            if (dxTexture) {
                cmdList->SetGraphicsRootDescriptorTable(1, dxTexture->GetGpuDescriptorHandle());
            } else {
                cmdList->SetGraphicsRootDescriptorTable(1, context->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
            }

            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmdList->DrawInstanced(6, 1, 0, 0);
        }
        return;
#endif

        m_2DShader->SetMat4("model", model);
        m_2DShader->SetVec4("spriteColor", glm::vec4(1.0f));
        m_2DShader->SetInt("useTexture", 1);

        textTex.texture->Bind(0);
        m_2DShader->SetInt("text", 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        textTex.texture->Unbind();
    }

    void Renderer::EndUI() {
        m_QuadVertexArray->Unbind();
#ifndef VECTOR_BUILD_DIRECTX
        glEnable(GL_DEPTH_TEST);
#endif
    }

    void Renderer::DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
#ifdef VECTOR_BUILD_DIRECTX
        return;
#endif
        m_2DShader->Bind();
#ifndef VECTOR_BUILD_DIRECTX
        glDisable(GL_DEPTH_TEST);
#endif

        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, -1.0f, 1.0f);
        m_2DShader->SetMat4("projection", projection);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(w, h, 1.0f));
        m_2DShader->SetMat4("model", model);

        m_2DShader->SetVec4("spriteColor", glm::vec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f));
        m_2DShader->SetInt("useTexture", 0);

#ifndef VECTOR_BUILD_DIRECTX
        m_QuadVertexArray->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_QuadVertexArray->Unbind();

        glEnable(GL_DEPTH_TEST);
#endif
    }

    void Renderer::DrawText(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b, int fontSize) {
        VECTOR_LOG_WARN("Legacy DrawText called! Use DrawUIText instead.");
    }

} // namespace VECTOR
