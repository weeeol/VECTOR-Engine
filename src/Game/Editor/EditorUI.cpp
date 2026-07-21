#include "Game/Editor/EditorUI.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include "Engine/ECS/Components.hpp"
#include "Engine/Core/Application.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace Game {

    EditorUI::EditorUI(VECTOR::Registry& registry, VECTOR::Renderer* renderer)
        : m_Registry(registry), m_Renderer(renderer)
    {
    }

    EditorUI::~EditorUI()
    {
    }

    EditorAction EditorUI::Render(bool isPlaying)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit"))
                {
                    VECTOR::Application::Get().Quit();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        DrawDockspace();
        EditorAction action = DrawToolbar(isPlaying);
        DrawHierarchy();
        DrawInspector();
        DrawProject();
        return action;
    }

    EditorAction EditorUI::DrawToolbar(bool isPlaying)
    {
        EditorAction action = EditorAction::None;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        
        ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        
        ImGui::Begin("##Toolbar", nullptr, toolbarFlags);
        
        float size = ImGui::GetWindowHeight() - 4.0f;
        ImGui::SameLine((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));
        
        if (isPlaying) {
            if (ImGui::Button("Stop", ImVec2(size * 3, size))) {
                action = EditorAction::Stop;
            }
        } else {
            if (ImGui::Button("Play", ImVec2(size * 3, size))) {
                action = EditorAction::Play;
            }
        }
        
        ImGui::End();
        
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        
        return action;
    }

    void EditorUI::DrawDockspace()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
        
        ImGui::Begin("DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(2);
        
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        
        if (m_FirstLayout) {
            m_FirstLayout = false;
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
            
            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
            ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
            ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, nullptr, &dock_main_id);
            ImGuiID dock_id_top = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.06f, nullptr, &dock_main_id);
            
            ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left);
            ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
            ImGui::DockBuilderDockWindow("Project", dock_id_bottom);
            ImGui::DockBuilderDockWindow("##Toolbar", dock_id_top);
            
            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();
    }

    void EditorUI::DrawHierarchy()
    {
        ImGui::Begin("Hierarchy");

        const auto& activeEntities = m_Registry.GetActiveEntities();
        for (VECTOR::Entity entity : activeEntities)
        {
            std::string label = "Entity " + std::to_string(entity);
            if (m_Registry.HasComponent<VECTOR::TagComponent>(entity))
            {
                label = m_Registry.GetComponent<VECTOR::TagComponent>(entity).tag;
            }

            ImGuiTreeNodeFlags flags = ((m_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            
            ImGui::TreeNodeEx((void*)(intptr_t)entity, flags, "%s", label.c_str());
            if (ImGui::IsItemClicked())
            {
                m_SelectedEntity = entity;
            }
        }

        ImGui::End();
    }

    void EditorUI::DrawInspector()
    {
        ImGui::Begin("Inspector");

        if (m_SelectedEntity != (VECTOR::Entity)-1)
        {
            const auto& activeEntities = m_Registry.GetActiveEntities();
            if (std::find(activeEntities.begin(), activeEntities.end(), m_SelectedEntity) == activeEntities.end())
            {
                m_SelectedEntity = (VECTOR::Entity)-1;
                ImGui::Text("Invalid Entity Selected");
                ImGui::End();
                return;
            }

            if (m_Registry.HasComponent<VECTOR::TagComponent>(m_SelectedEntity))
            {
                auto& tag = m_Registry.GetComponent<VECTOR::TagComponent>(m_SelectedEntity).tag;
                char buffer[256];
                strncpy(buffer, tag.c_str(), sizeof(buffer));
                if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
                {
                    tag = std::string(buffer);
                }
                ImGui::Separator();
            }

            if (m_Registry.HasComponent<VECTOR::TransformComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& transform = m_Registry.GetComponent<VECTOR::TransformComponent>(m_SelectedEntity);
                    ImGui::DragFloat3("Position", glm::value_ptr(transform.position), 0.1f);
                    
                    glm::vec3 euler = glm::degrees(glm::eulerAngles(transform.rotation));
                    if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 1.0f)) {
                        transform.rotation = glm::quat(glm::radians(euler));
                    }
                    
                    ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.1f);
                }
            }

            if (m_Registry.HasComponent<VECTOR::PointLightComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& light = m_Registry.GetComponent<VECTOR::PointLightComponent>(m_SelectedEntity);
                    ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
                    ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 100.0f);
                    ImGui::DragFloat("Radius", &light.radius, 0.1f, 0.0f, 500.0f);
                }
            }

            if (m_Registry.HasComponent<VECTOR::CameraComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& cam = m_Registry.GetComponent<VECTOR::CameraComponent>(m_SelectedEntity);
                    ImGui::DragFloat("FOV", &cam.fov, 1.0f, 10.0f, 120.0f);
                }
            }
        }
        else
        {
            ImGui::Text("No entity selected");
        }

        ImGui::End();
    }

    void EditorUI::DrawProject()
    {
        ImGui::Begin("Project");
        ImGui::Text("Assets");
        ImGui::Separator();
        ImGui::TextDisabled("Right click to create new assets...");
        
        // Dummy assets
        ImGui::Selectable("  Scenes", false);
        ImGui::Selectable("  Models", false);
        ImGui::Selectable("  Scripts", false);
        ImGui::End();
    }
}
