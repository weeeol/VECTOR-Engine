#pragma once

#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "Engine/Graphics/Bone.hpp"

namespace VECTOR {

    struct AssimpNodeData {
        glm::mat4 transformation;
        std::string name;
        int childrenCount;
        std::vector<AssimpNodeData> children;
    };

    struct BoneInfo {
        int id;
        glm::mat4 offset;
    };

    class Model; // Forward declare

    class Animation {
    public:
        Animation() = default;
        Animation(const std::string& animationPath, Model* model);

        ~Animation() {}

        Bone* FindBone(const std::string& name);

        inline float GetTicksPerSecond() const { return m_TicksPerSecond; }
        inline float GetDuration() const { return m_Duration; }
        inline const AssimpNodeData& GetRootNode() const { return m_RootNode; }
        inline const std::map<std::string, BoneInfo>& GetBoneIDMap() const { return m_BoneInfoMap; }

    private:
        void ReadMissingBones(const aiAnimation* animation, Model& model);
        void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src);

        float m_Duration;
        int m_TicksPerSecond;
        std::vector<Bone> m_Bones;
        AssimpNodeData m_RootNode;
        std::map<std::string, BoneInfo> m_BoneInfoMap;
    };

} // namespace VECTOR
