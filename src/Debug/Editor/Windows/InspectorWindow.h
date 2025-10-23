#pragma once

#include <entt/entt.hpp>
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include <functional>
#include <unordered_map>
#include <string>
#include "raylib.h"

#include "EditorWindow.h"

namespace Struktur
{
    namespace Component
    {
        struct LocalTransform;
        struct Sprite;
    }

    namespace Debug
    {
        class HierarchyWindow;
        
        // Type-erased component renderer function
        using ComponentRenderer = std::function<void(entt::registry&, entt::entity)>;
        
        class InspectorWindow : public EditorWindow
        {
        public:
            InspectorWindow(HierarchyWindow* hierarchyWindow)
                : EditorWindow("Inspector")
                , m_hierarchyWindow(hierarchyWindow)
            {
                // Register default component renderers
                RegisterDefaultRenderers();
            }
            
            void Render(GameContext& context) override;
            
            // Register a custom component renderer
            template<typename T>
            void RegisterComponentRenderer(const std::string& componentName, 
                                        std::function<void(T&, entt::registry&, entt::entity)> renderFunc)
            {
                m_componentRenderers[componentName] = [renderFunc](entt::registry& registry, entt::entity entity)
                {
                    if (auto* component = registry.try_get<T>(entity))
                    {
                        renderFunc(*component, registry, entity);
                    }
                };
            }
            
        private:
            void RegisterDefaultRenderers();
            
            // Render entity info header
            void RenderEntityHeader(GameContext& context, entt::entity entity);
            
            // Render all components of an entity
            void RenderComponents(GameContext& context, entt::entity entity);
            
            // Component-specific renderers
            void RenderLocalTransformComponent(Component::LocalTransform& transform, 
                                            entt::registry& registry, 
                                            entt::entity entity);
            
            void RenderSpriteComponent(Component::Sprite& sprite, 
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
            std::unordered_map<std::string, ComponentRenderer> m_componentRenderers;
        };
    }
}