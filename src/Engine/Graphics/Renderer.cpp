#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_ttf.h>

namespace VECTOR {

    // Simple 3D Shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        
        out vec3 FragPos;
        out vec3 Normal;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        
        uniform vec3 objectColor;
        uniform vec3 lightPos;
        uniform vec3 lightColor;
        
        void main() {
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * lightColor;
            
            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            
            vec3 result = (ambient + diffuse) * objectColor;
            FragColor = vec4(result, 1.0);
        }
    )";

    // Simple 2D Shader
    const char* vertexShader2DSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoords;

        out vec2 TexCoords;

        uniform mat4 projection;
        uniform mat4 model;

        void main() {
            TexCoords = aTexCoords;
            gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
        }
    )";

    const char* fragmentShader2DSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 color;

        uniform sampler2D text;
        uniform vec4 spriteColor;
        uniform bool useTexture;

        void main() {
            if (useTexture) {
                vec4 sampled = texture(text, TexCoords);
                color = spriteColor * sampled;
            } else {
                color = spriteColor;
            }
        }
    )";

    Renderer::Renderer() : m_Window(nullptr), m_GLContext(nullptr), m_ShaderProgram(0), m_2DShaderProgram(0) {}

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
        
        m_ShaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
        m_2DShaderProgram = CreateShaderProgram(vertexShader2DSource, fragmentShader2DSource);

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
        if (m_QuadVAO) glDeleteVertexArrays(1, &m_QuadVAO);
        if (m_QuadVBO) glDeleteBuffers(1, &m_QuadVBO);
        if (m_ShaderProgram) glDeleteProgram(m_ShaderProgram);
        if (m_2DShaderProgram) glDeleteProgram(m_2DShaderProgram);

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

    void Renderer::SetViewProjection(const glm::mat4& view, const glm::mat4& projection) {
        m_ViewMatrix = view;
        m_ProjectionMatrix = projection;
    }

    void Renderer::DrawMesh(unsigned int VAO, int indexCount, const glm::mat4& model, const glm::vec3& color) {
        glUseProgram(m_ShaderProgram);

        unsigned int modelLoc = glGetUniformLocation(m_ShaderProgram, "model");
        unsigned int viewLoc  = glGetUniformLocation(m_ShaderProgram, "view");
        unsigned int projLoc  = glGetUniformLocation(m_ShaderProgram, "projection");
        unsigned int colorLoc = glGetUniformLocation(m_ShaderProgram, "objectColor");
        
        unsigned int lightPosLoc = glGetUniformLocation(m_ShaderProgram, "lightPos");
        unsigned int lightColLoc = glGetUniformLocation(m_ShaderProgram, "lightColor");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_ViewMatrix));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_ProjectionMatrix));
        glUniform3fv(colorLoc, 1, glm::value_ptr(color));
        
        glUniform3f(lightPosLoc, 0.0f, 10.0f, 10.0f);
        glUniform3f(lightColLoc, 1.0f, 1.0f, 1.0f);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void Renderer::DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        glUseProgram(m_2DShaderProgram);
        glDisable(GL_DEPTH_TEST);

        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, -1.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(m_2DShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(w, h, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(m_2DShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glUniform4f(glGetUniformLocation(m_2DShaderProgram, "spriteColor"), r/255.0f, g/255.0f, b/255.0f, a/255.0f);
        glUniform1i(glGetUniformLocation(m_2DShaderProgram, "useTexture"), 0);

        glBindVertexArray(m_QuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
    }

    void Renderer::DrawText(const std::string& text, int x, int y, uint8_t r, uint8_t g, uint8_t b, int fontSize) {
        if (text.empty()) return;

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

        glUseProgram(m_2DShaderProgram);
        glDisable(GL_DEPTH_TEST);

        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, -1.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(m_2DShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(surface->w, surface->h, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(m_2DShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glUniform4f(glGetUniformLocation(m_2DShaderProgram, "spriteColor"), 1.0f, 1.0f, 1.0f, 1.0f);
        glUniform1i(glGetUniformLocation(m_2DShaderProgram, "useTexture"), 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(m_2DShaderProgram, "text"), 0);

        glBindVertexArray(m_QuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &texture);
        SDL_FreeSurface(surface);
    }

    unsigned int Renderer::CompileShader(unsigned int type, const std::string& source) {
        unsigned int id = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        int result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE) {
            int length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            char* message = (char*)alloca(length * sizeof(char));
            glGetShaderInfoLog(id, length, &length, message);
            VECTOR_LOG_ERROR(std::string("Failed to compile shader! ") + message);
            glDeleteShader(id);
            return 0;
        }

        return id;
    }

    unsigned int Renderer::CreateShaderProgram(const std::string& vertexSrc, const std::string& fragmentSrc) {
        unsigned int program = glCreateProgram();
        unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
        unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        glValidateProgram(program);

        glDeleteShader(vs);
        glDeleteShader(fs);

        return program;
    }

} // namespace VECTOR
