#include "Engine/Graphics/Model.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    Model::Model(const std::string& path) {
        LoadModel(path);
    }

    void Model::Draw() const {
        for (const auto& mesh : m_Meshes) {
            mesh->Draw();
        }
    }

    void Model::LoadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            VECTOR_LOG_ERROR("ERROR::ASSIMP::" + std::string(importer.GetErrorString()));
            return;
        }

        ProcessNode(scene->mRootNode, scene);
    }

    void Model::ProcessNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            m_Meshes.push_back(ProcessMesh(mesh, scene));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene);
        }
    }

    void Model::SetVertexBoneDataToDefault(Vertex& vertex) {
        vertex.BoneIDs = glm::ivec4(-1);
        vertex.Weights = glm::vec4(0.0f);
    }

    void Model::SetVertexBoneData(Vertex& vertex, int boneID, float weight) {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if (vertex.BoneIDs[i] < 0) {
                vertex.Weights[i] = weight;
                vertex.BoneIDs[i] = boneID;
                break;
            }
        }
    }

    void Model::ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene) {
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
            
            if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end()) {
                BoneInfo newBoneInfo;
                newBoneInfo.id = m_BoneCount;
                
                auto aiOffsetMat = mesh->mBones[boneIndex]->mOffsetMatrix;
                newBoneInfo.offset = glm::mat4(
                    aiOffsetMat.a1, aiOffsetMat.b1, aiOffsetMat.c1, aiOffsetMat.d1,
                    aiOffsetMat.a2, aiOffsetMat.b2, aiOffsetMat.c2, aiOffsetMat.d2,
                    aiOffsetMat.a3, aiOffsetMat.b3, aiOffsetMat.c3, aiOffsetMat.d3,
                    aiOffsetMat.a4, aiOffsetMat.b4, aiOffsetMat.c4, aiOffsetMat.d4
                );

                m_BoneInfoMap[boneName] = newBoneInfo;
                boneID = m_BoneCount;
                m_BoneCount++;
            } else {
                boneID = m_BoneInfoMap[boneName].id;
            }
            
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;
            
            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                if (vertexId <= vertices.size()) {
                    SetVertexBoneData(vertices[vertexId], boneID, weight);
                }
            }
        }
    }

    std::shared_ptr<Mesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            SetVertexBoneDataToDefault(vertex);
            
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            if (mesh->HasNormals()) {
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            } else {
                vertex.Normal = glm::vec3(0.0f);
            }

            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        ExtractBoneWeightForVertices(vertices, mesh, scene);

        return Mesh::Create(vertices, indices);
    }

} // namespace VECTOR
