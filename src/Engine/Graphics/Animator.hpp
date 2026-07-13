#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/Texture.hpp"

namespace VECTOR {

    class Animator {
    public:
        Animator(Texture* texture, int frameWidth, int frameHeight, int numFrames, float frameDuration);
        
        void Update(float deltaTime);
        void Render(Renderer* renderer, int x, int y, int width = -1, int height = -1);

        void Play();
        void Pause();
        void Reset();
        
        void SetFrameDuration(float duration) { m_FrameDuration = duration; }

    private:
        Texture* m_Texture;
        int m_FrameWidth;
        int m_FrameHeight;
        int m_NumFrames;
        float m_FrameDuration;
        
        float m_CurrentTimer;
        int m_CurrentFrame;
        bool m_IsPlaying;
    };

} // namespace VECTOR
