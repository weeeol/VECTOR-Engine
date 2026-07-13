#include "Engine/Graphics/Animator.hpp"
#include <SDL.h>

namespace VECTOR {

    Animator::Animator(Texture* texture, int frameWidth, int frameHeight, int numFrames, float frameDuration)
        : m_Texture(texture), m_FrameWidth(frameWidth), m_FrameHeight(frameHeight),
          m_NumFrames(numFrames), m_FrameDuration(frameDuration), m_CurrentTimer(0.0f),
          m_CurrentFrame(0), m_IsPlaying(true)
    {
    }

    void Animator::Update(float deltaTime) {
        if (!m_IsPlaying || m_NumFrames <= 1) return;

        m_CurrentTimer += deltaTime;
        if (m_CurrentTimer >= m_FrameDuration) {
            m_CurrentTimer -= m_FrameDuration;
            m_CurrentFrame = (m_CurrentFrame + 1) % m_NumFrames;
        }
    }

    void Animator::Render(Renderer* renderer, int x, int y, int width, int height) {
        if (!m_Texture || !m_Texture->IsValid()) return;

        SDL_Rect srcRect;
        srcRect.x = m_CurrentFrame * m_FrameWidth;
        srcRect.y = 0;
        srcRect.w = m_FrameWidth;
        srcRect.h = m_FrameHeight;

        SDL_Rect destRect;
        destRect.x = x;
        destRect.y = y;
        destRect.w = (width == -1) ? m_FrameWidth : width;
        destRect.h = (height == -1) ? m_FrameHeight : height;

        SDL_RenderCopy(renderer->GetSDLRenderer(), m_Texture->GetSDLTexture(), &srcRect, &destRect);
    }

    void Animator::Play() {
        m_IsPlaying = true;
    }

    void Animator::Pause() {
        m_IsPlaying = false;
    }

    void Animator::Reset() {
        m_CurrentFrame = 0;
        m_CurrentTimer = 0.0f;
    }

} // namespace VECTOR
