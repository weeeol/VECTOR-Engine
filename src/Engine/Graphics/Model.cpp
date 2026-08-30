#include "Engine/Graphics/Model.hpp"
#include "Engine/Core/Logger.hpp"
#include "Engine/Graphics/Material.hpp"
#include "Engine/Core/ResourceManager.hpp"
#include <filesystem>

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

        ProcessMaterials(scene, path);
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

    void Model::ProcessMaterials(const aiScene* scene, const std::string& modelPath) {
        std::string directory = std::filesystem::path(modelPath).parent_path().string();
        if (!directory.empty() && directory.back() != '/' && directory.back() != '\\') {
            directory += "/";
        }

        m_Materials.reserve(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* aiMat = scene->mMaterials[i];
            auto mat = std::make_shared<Material>();
            
            // Assign default shader
            mat->shader = ResourceManager::Get().GetShader("Main3D");

            // Diffuse / Albedo
            aiString str;
            if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &str) == aiReturn_SUCCESS) {
                std::string p = directory + str.C_Str();
                mat->albedoTexture = ResourceManager::Get().LoadTexture2D(p, p);
            }
            // Base Color
            if (aiMat->GetTexture(aiTextureType_BASE_COLOR, 0, &str) == aiReturn_SUCCESS) {
                std::string p = directory + str.C_Str();
                mat->albedoTexture = ResourceManager::Get().LoadTexture2D(p, p);
            }

            // Normal
            if (aiMat->GetTexture(aiTextureType_NORMALS, 0, &str) == aiReturn_SUCCESS) {
                std::string p = directory + str.C_Str();
                mat->normalTexture = ResourceManager::Get().LoadTexture2D(p, p);
            }

            // Metallic/Roughness (usually in unknown for GLTF, but let's check standard types too)
            if (aiMat->GetTexture(aiTextureType_UNKNOWN, 0, &str) == aiReturn_SUCCESS) { // GLTF MR map
                std::string p = directory + str.C_Str();
                mat->metallicRoughnessTexture = ResourceManager::Get().LoadTexture2D(p, p);
            }

            // Ambient Occlusion
            if (aiMat->GetTexture(aiTextureType_AMBIENT, 0, &str) == aiReturn_SUCCESS) {
                std::string p = directory + str.C_Str();
                mat->aoTexture = ResourceManager::Get().LoadTexture2D(p, p);
            }

            // Properties
            aiColor4D color;
            if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &color) == aiReturn_SUCCESS) {
                mat->albedoColor = glm::vec4(color.r, color.g, color.b, color.a);
            }

            float roughness;
            if (aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness) == aiReturn_SUCCESS) {
                mat->roughness = roughness;
            }

            float metallic;
            if (aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &metallic) == aiReturn_SUCCESS) {
                mat->metallic = metallic;
            }

            m_Materials.push_back(mat);
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
