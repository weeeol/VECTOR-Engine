#include "Engine/Graphics/SkeletalAnimator.hpp"
#include "Engine/Graphics/Model.hpp" // For MAX_BONE_INFLUENCE conceptually, but maybe not needed directly here

namespace VECTOR {

    SkeletalAnimator::SkeletalAnimator(Animation* animation) {
        m_CurrentTime = 0.0f;
        m_CurrentAnimation = animation;

        m_FinalBoneMatrices.reserve(100);
        for (int i = 0; i < 100; i++)
            m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
    }

    void SkeletalAnimator::UpdateAnimation(float dt) {
        m_DeltaTime = dt;
        if (m_CurrentAnimation) {
            m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
            m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
            CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
        }
    }

    void SkeletalAnimator::PlayAnimation(Animation* pAnimation) {
        m_CurrentAnimation = pAnimation;
        m_CurrentTime = 0.0f;
    }

    void SkeletalAnimator::CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform) {
        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;

        Bone* bone = m_CurrentAnimation->FindBone(nodeName);

        if (bone) {
            bone->Update(m_CurrentTime);
            nodeTransform = bone->GetLocalTransform();
        }

        glm::mat4 globalTransformation = parentTransform * nodeTransform;

        auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
        if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
            int index = boneInfoMap[nodeName].id;
            glm::mat4 offset = boneInfoMap[nodeName].offset;
            m_FinalBoneMatrices[index] = globalTransformation * offset;
        }

        for (int i = 0; i < node->childrenCount; i++) {
            CalculateBoneTransform(&node->children[i], globalTransformation);
        }
    }

} // namespace VECTOR
