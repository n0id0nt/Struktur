#pragma once

#include <entt/entt.hpp>
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include <functional>
#include <unordered_map>
#include <string>
#include "raylib.h"

#include "Debug/Editor/Windows/EditorWindow.h"

namespace Struktur
{
    namespace Component
    {
        struct LocalTransform;
        struct Sprite;
        struct Shader;
    }

    namespace Debug
    {
        class HierarchyWindow;
        class PreviewWindow;
        
        // Type-erased component renderer function
        using ComponentRenderer = std::function<void(entt::registry&, entt::entity)>;
        
        class InspectorWindow : public EditorWindow
        {
        public:
            InspectorWindow(HierarchyWindow* hierarchyWindow, PreviewWindow* previewWindow);
            
            void Render(GameContext& context) override;
            
            // Register a custom component renderer //TODO Finish this off or remove it.
            template<typename T>
            void RegisterComponentRenderer(const std::string& componentName, 
                                        std::function<void(GameContext& context, T&, entt::registry&, entt::entity)> renderFunc)
            {
                //m_componentRenderers[componentName] = [renderFunc](GameContext& context, entt::registry& registry, entt::entity entity)
                //{
                //    if (auto* component = registry.try_get<T>(entity))
                //    {
                //        renderFunc(context, *component, registry, entity);
                //    }
                //};
            }
            
        private:
            void RegisterDefaultRenderers();
            
            // Render entity info header
            void RenderEntityHeader(GameContext& context, entt::entity entity);
            
            // Render all components of an entity
            void RenderComponents(GameContext& context, entt::entity entity);
            
            // Component-specific renderers
            void RenderLocalTransformComponent(GameContext& context, Component::LocalTransform& transform, 
                                            entt::registry& registry, 
                                            entt::entity entity);
            
            void RenderSpriteComponent(GameContext& context, Component::Sprite& sprite, 
                                    entt::registry& registry, 
                                    entt::entity entity);

            void RenderShaderComponent(GameContext& context, Component::Shader& shader,
                                  entt::registry& registry,
                                  entt::entity entity);
            
            // Helper functions for rendering common data types
            bool RenderVec2(const char* label, glm::vec2& vec);
            bool RenderVec3(const char* label, glm::vec3& vec);
            bool RenderVec4(const char* label, glm::vec4& vec);
            bool RenderQuat(const char* label, glm::quat& quat);
            bool RenderColor(const char* label, ::Color& color);
            
        private:
            HierarchyWindow* m_hierarchyWindow;
            PreviewWindow* m_previewWindow;
            std::unordered_map<std::string, ComponentRenderer> m_componentRenderers;
        };
    }
}