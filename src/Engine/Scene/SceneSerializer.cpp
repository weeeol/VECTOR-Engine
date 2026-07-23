#include "Engine/Scene/SceneSerializer.hpp"
#include "Engine/ECS/Components.hpp"
#include "Engine/Core/Logger.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace glm {
    void to_json(json& j, const vec3& v) {
        j = json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
    }

    void from_json(const json& j, vec3& v) {
        j.at("x").get_to(v.x);
        j.at("y").get_to(v.y);
        j.at("z").get_to(v.z);
    }
    
    void to_json(json& j, const quat& q) {
        j = json{{"w", q.w}, {"x", q.x}, {"y", q.y}, {"z", q.z}};
    }

    void from_json(const json& j, quat& q) {
        j.at("w").get_to(q.w);
        j.at("x").get_to(q.x);
        j.at("y").get_to(q.y);
        j.at("z").get_to(q.z);
    }
}

namespace VECTOR {

    SceneSerializer::SceneSerializer(Registry* registry)
        : m_Registry(registry) {
    }

    void SceneSerializer::Serialize(const std::string& filepath) {
        json sceneData = json::array();

        const auto& activeEntities = m_Registry->GetActiveEntities();
        for (Entity entity : activeEntities) {
            json entityData;
            entityData["Entity"] = static_cast<uint32_t>(entity);

            if (m_Registry->HasComponent<TagComponent>(entity)) {
                auto& tc = m_Registry->GetComponent<TagComponent>(entity);
                entityData["TagComponent"] = {
                    {"tag", tc.tag}
                };
            }

            if (m_Registry->HasComponent<TransformComponent>(entity)) {
                auto& tc = m_Registry->GetComponent<TransformComponent>(entity);
                entityData["TransformComponent"] = {
                    {"position", tc.position},
                    {"rotation", tc.rotation},
                    {"scale", tc.scale}
                };
            }

            if (m_Registry->HasComponent<CameraComponent>(entity)) {
                auto& cc = m_Registry->GetComponent<CameraComponent>(entity);
                entityData["CameraComponent"] = {
                    {"fov", cc.fov},
                    {"front", cc.front},
                    {"up", cc.up},
                    {"right", cc.right}
                };
            }

            if (m_Registry->HasComponent<PointLightComponent>(entity)) {
                auto& lc = m_Registry->GetComponent<PointLightComponent>(entity);
                entityData["PointLightComponent"] = {
                    {"color", lc.color},
                    {"intensity", lc.intensity},
                    {"radius", lc.radius}
                };
            }
            
            // We'll skip MeshComponent and RenderComponent for now since they contain heavy shared_ptr resources
            // For a full engine, we'd serialize asset GUIDs or file paths

            sceneData.push_back(entityData);
        }

        std::ofstream fout(filepath);
        if (fout.is_open()) {
            fout << sceneData.dump(4); // 4 spaces indent
            VECTOR_LOG_INFO("Successfully serialized scene to " + filepath);
        } else {
            VECTOR_LOG_ERROR("Failed to open file for scene serialization: " + filepath);
        }
    }

    bool SceneSerializer::Deserialize(const std::string& filepath) {
        std::ifstream stream(filepath);
        if (!stream.is_open()) {
            VECTOR_LOG_ERROR("Failed to open file for scene deserialization: " + filepath);
            return false;
        }

        json sceneData;
        try {
            stream >> sceneData;
        } catch (json::parse_error& e) {
            VECTOR_LOG_ERROR("JSON parse error in " + filepath + ": " + e.what());
            return false;
        }

        if (!sceneData.is_array()) {
            VECTOR_LOG_ERROR("Invalid scene file format: root is not an array");
            return false;
        }
        
        // Before deserializing, we would usually clear the registry. 
        // m_Registry->Clear(); // Assuming we had a clear function!

        for (const auto& entityData : sceneData) {
            Entity entity = m_Registry->CreateEntity();

            if (entityData.contains("TagComponent")) {
                m_Registry->AddComponent(entity, TagComponent{entityData["TagComponent"]["tag"].get<std::string>()});
            } else {
                m_Registry->AddComponent(entity, TagComponent{"DeserializedEntity"});
            }

            if (entityData.contains("TransformComponent")) {
                TransformComponent tc;
                tc.position = entityData["TransformComponent"]["position"].get<glm::vec3>();
                tc.rotation = entityData["TransformComponent"]["rotation"].get<glm::quat>();
                tc.scale = entityData["TransformComponent"]["scale"].get<glm::vec3>();
                m_Registry->AddComponent(entity, tc);
            }

            if (entityData.contains("CameraComponent")) {
                CameraComponent cc;
                cc.fov = entityData["CameraComponent"]["fov"].get<float>();
                cc.front = entityData["CameraComponent"]["front"].get<glm::vec3>();
                cc.up = entityData["CameraComponent"]["up"].get<glm::vec3>();
                cc.right = entityData["CameraComponent"]["right"].get<glm::vec3>();
                m_Registry->AddComponent(entity, cc);
            }

            if (entityData.contains("PointLightComponent")) {
                PointLightComponent lc;
                lc.color = entityData["PointLightComponent"]["color"].get<glm::vec3>();
                lc.intensity = entityData["PointLightComponent"]["intensity"].get<float>();
                lc.radius = entityData["PointLightComponent"]["radius"].get<float>();
                m_Registry->AddComponent(entity, lc);
            }
        }
        
        VECTOR_LOG_INFO("Successfully deserialized scene from " + filepath);
        return true;
    }

}
