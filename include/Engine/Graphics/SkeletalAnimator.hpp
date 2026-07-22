#pragma once

#include <vector>
#include <map>
#include <glm/glm.hpp>
#include "Engine/Graphics/Animation.hpp"

namespace VECTOR {

    class SkeletalAnimator {
    public:
        SkeletalAnimator(Animation* animation);
        ~SkeletalAnimator() = default;

        void UpdateAnimation(float dt);
        void PlayAnimation(Animation* pAnimation);
        void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);
        const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return m_FinalBoneMatrices; }

    private:
        std::vector<glm::mat4> m_FinalBoneMatrices;
        Animation* m_CurrentAnimation;
        float m_CurrentTime;
        float m_DeltaTime;
    };

} // namespace VECTOR
