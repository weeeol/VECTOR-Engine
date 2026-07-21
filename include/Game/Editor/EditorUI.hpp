#pragma once
#include "Engine/ECS/ECS.hpp"
#include "Engine/Graphics/Renderer.hpp"

namespace Game {

    enum class EditorAction { None, Play, Stop };

    class EditorUI {
    public:
        EditorUI(VECTOR::Registry& registry, VECTOR::Renderer* renderer);
        ~EditorUI();

        EditorAction Render(bool isPlaying);
        VECTOR::Entity GetSelectedEntity() const { return m_SelectedEntity; }
        
        bool IsSceneHovered() const { return m_SceneHovered; }
        bool IsSceneFocused() const { return m_SceneFocused; }
        
    private:
        EditorAction DrawToolbar(bool isPlaying);
        void DrawDockspace();
        void DrawHierarchy();
        void DrawInspector();
        void DrawProject();
        void DrawScene();

        VECTOR::Registry& m_Registry;
        VECTOR::Renderer* m_Renderer;
        
        bool m_SceneHovered = false;
        bool m_SceneFocused = false;
        
        VECTOR::Entity m_SelectedEntity = 0;
        bool m_FirstLayout = true;
    };

}
