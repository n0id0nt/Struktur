#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <string>
#include <unordered_map>

#include "Debug/Editor/Windows/EditorWindow.h"
#include "Engine/Util/MathUtil.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."

namespace Struktur
{
namespace Component
{
struct Transform;
struct Sprite;
struct Shader;
}  // namespace Component

namespace UI
{
class UIElement;
}

namespace Debug
{
class HierarchyWindow;
class UIHierarchyWindow;
class PreviewWindow;

// Type-erased component renderer function
using ComponentRenderer = std::function<void(entt::registry&, entt::entity)>;

class InspectorWindow : public EditorWindow
{
   private:
	enum class InspectorDisplayType
	{
		None,
		Entity,
		UIElement,

		Count
	};

   public:
	InspectorWindow(HierarchyWindow* hierarchyWindow, UIHierarchyWindow* uiHierarchyWindow,
	                PreviewWindow* previewWindow);

	void Render(GameContext& context) override;

	// Register a custom component renderer //TODO Finish this off or remove it.
	template <typename T>
	void RegisterComponentRenderer(
	    const std::string& componentName,
	    std::function<void(GameContext& context, T&, entt::registry&, entt::entity)> renderFunc)
	{
		// m_componentRenderers[componentName] = [renderFunc](GameContext& context, entt::registry& registry,
		// entt::entity entity)
		//{
		//     if (auto* component = registry.try_get<T>(entity))
		//     {
		//         renderFunc(context, *component, registry, entity);
		//     }
		// };
	}

   private:
	void RegisterDefaultRenderers();

	// Entity rendering
	void RenderEntityInspector(GameContext& context, entt::entity entity);
	void RenderEntityHeader(GameContext& context, entt::entity entity);
	void RenderComponents(GameContext& context, entt::entity entity);

	// UI Element rendering
	void RenderUIElementInspector(GameContext& context, UI::UIElement* element);
	void RenderUIElementHeader(UI::UIElement* element);
	void RenderUIElementProperties(UI::UIElement* element);

	// Component-specific renderers
	void RenderLocalTransformComponent(GameContext& context, Component::Transform& transform,
	                                   entt::registry& registry, entt::entity entity);

	void RenderSpriteComponent(GameContext& context, Component::Sprite& sprite, entt::registry& registry,
	                           entt::entity entity);

	void RenderShaderComponent(GameContext& context, Component::Shader& shader, entt::registry& registry,
	                           entt::entity entity);

	// Helper functions for rendering common data types
	bool RenderVec2(const char* label, glm::vec2& vec);
	bool RenderVec3(const char* label, glm::vec3& vec);
	bool RenderVec4(const char* label, glm::vec4& vec);
	bool RenderQuat(const char* label, glm::quat& quat);
	bool RenderColor(const char* label, Util::Math::Color& color);

	HierarchyWindow* m_hierarchyWindow;
	UIHierarchyWindow* m_uiHierarchyWindow;
	PreviewWindow* m_previewWindow;
	std::unordered_map<std::string, ComponentRenderer> m_componentRenderers;
	InspectorDisplayType m_displayType;
	UI::UIElement* m_selectedUIElement;
	entt::entity m_selectedEntity;
};
}  // namespace Debug
}  // namespace Struktur
