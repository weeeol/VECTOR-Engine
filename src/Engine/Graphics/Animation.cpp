#include "Engine/Graphics/Animation.hpp"
#include "Engine/Graphics/Model.hpp"
#include "Engine/Core/Logger.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace VECTOR {

    Animation::Animation(const std::string& animationPath, Model* model) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
        if (!scene || !scene->mRootNode) {
            VECTOR_LOG_ERROR("Assimp Animation Loading error: " + std::string(importer.GetErrorString()));
            return;
        }

        if (scene->mNumAnimations == 0) {
            VECTOR_LOG_ERROR("No animations found in file: " + animationPath);
            return;
        }

        auto animation = scene->mAnimations[0];
        m_Duration = static_cast<float>(animation->mDuration);
        m_TicksPerSecond = static_cast<int>(animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 24.0f);

        ReadHierarchyData(m_RootNode, scene->mRootNode);
        ReadMissingBones(animation, *model);
    }

    Bone* Animation::FindBone(const std::string& name) {
        auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
            [&](const Bone& bone) {
                return bone.GetBoneName() == name;
            }
        );
        if (iter == m_Bones.end()) return nullptr;
        else return &(*iter);
    }

    void Animation::ReadMissingBones(const aiAnimation* animation, Model& model) {
        int size = animation->mNumChannels;

        auto& boneInfoMap = model.GetBoneInfoMap();
        int& boneCount = model.GetBoneCount();

        for (int i = 0; i < size; i++) {
            auto channel = animation->mChannels[i];
            std::string boneName = channel->mNodeName.data;

            if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
                boneInfoMap[boneName].id = boneCount;
                boneCount++;
            }
            m_Bones.push_back(Bone(boneName, boneInfoMap[boneName].id, channel));
        }

        m_BoneInfoMap = boneInfoMap;
    }

    void Animation::ReadHierarchyData(AssimpNodeData& dest, const aiNode* src) {
        dest.name = src->mName.data;

        // Convert Assimp matrix to GLM
        const aiMatrix4x4& aiMat = src->mTransformation;
        dest.transformation = glm::mat4(
            aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
            aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
            aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
            aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
        );

        dest.childrenCount = src->mNumChildren;

        for (unsigned int i = 0; i < src->mNumChildren; i++) {
            AssimpNodeData newData;
            ReadHierarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }

} // namespace VECTOR
