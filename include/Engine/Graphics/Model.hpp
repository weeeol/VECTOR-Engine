#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include "Engine/Graphics/Mesh.hpp"
#include "Engine/Graphics/Animation.hpp" // For BoneInfo

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace VECTOR {

    class Model {
    public:
        Model(const std::string& path);
        ~Model() = default;

        void Draw() const;

        auto& GetBoneInfoMap() { return m_BoneInfoMap; }
        int& GetBoneCount() { return m_BoneCount; }

        const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_Meshes; }

    private:
        void LoadModel(const std::string& path);
        void ProcessNode(aiNode* node, const aiScene* scene);
        std::shared_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene);
        void SetVertexBoneDataToDefault(Vertex& vertex);
        void SetVertexBoneData(Vertex& vertex, int boneID, float weight);
        void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);

        std::vector<std::shared_ptr<Mesh>> m_Meshes;
        
        std::map<std::string, BoneInfo> m_BoneInfoMap;
        int m_BoneCount = 0;
    };

} // namespace VECTOR
