#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <string>
#include <unordered_map>

#include "Debug/Editor/Windows/EditorWindow.h"
#include "Engine/Util/Color.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."

namespace Struktur
{
namespace Animation
{
struct SpriteAnimation;
}  // namespace Animation

namespace Component
{
struct Active;
struct Transform;
struct Sprite;
struct Shader;
struct Camera;
struct ParticleEmitter;
struct PhysicsBody;
struct SpriteAnimation;
struct Level;
struct World;
struct TileMap;
struct WrenScript;
}  // namespace Component

namespace UI
{
class UIElement;
class UILabel;
class UIRichLabel;
class UITexture;
class UIScroll;
class UINineSlice;
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
	void RenderUIElementProperties(GameContext& context, UI::UIElement* element);

	// Type-specific UI element property sections - dispatched from RenderUIElementProperties by dynamic_cast,
	// same pattern RenderComponents uses for entity components. One method per concrete type regardless of size,
	// for the same reason every entity component gets its own Render*Component method even when small (see
	// RenderActiveComponent) - keeps each type's fields in one place rather than growing one giant function.
	// UIClip has no properties of its own beyond base UIElement (pure clipping boundary, see its class comment)
	// so it doesn't get a dedicated method - RenderUIElementProperties just notes that inline.
	void RenderUILabelProperties(UI::UILabel* label);
	void RenderUIRichLabelProperties(UI::UIRichLabel* richLabel);
	void RenderUITextureProperties(UI::UITexture* texture);
	void RenderUIScrollProperties(GameContext& context, UI::UIScroll* scroll);
	void RenderUINineSliceProperties(UI::UINineSlice* nineSlice);

	// Dropdown of every other UIElement currently in the tree (walked from UIManager's roots), for fields that
	// reference another element by pointer (navigation neighbors, UIScroll's scroll indicator) - there's no
	// other way to "author" a UIElement* from within an ImGui panel. Elements without an ID are listed by
	// address so they're still selectable, just not nameable - see the comment where this is used for why IDs
	// matter here more than they do elsewhere. Returns true and writes the newly picked element (nullptr for
	// "[None]") into value if the selection changed this frame; excludeSelf is omitted from the candidate list
	// so a field can't be pointed at its own owning element.
	bool RenderUIElementPicker(const char* label, GameContext& context, UI::UIElement* excludeSelf,
	                           UI::UIElement*& value);

	// Component-specific renderers
	void RenderActiveComponent(GameContext& context, Component::Active& active, entt::registry& registry,
	                           entt::entity entity);

	void RenderLocalTransformComponent(GameContext& context, Component::Transform& transform, entt::registry& registry,
	                                   entt::entity entity);

	void RenderSpriteComponent(GameContext& context, Component::Sprite& sprite, entt::registry& registry,
	                           entt::entity entity);

	void RenderShaderComponent(GameContext& context, Component::Shader& shader, entt::registry& registry,
	                           entt::entity entity);

	void RenderCameraComponent(GameContext& context, Component::Camera& camera, entt::registry& registry,
	                           entt::entity entity);

	void RenderParticleEmitterComponent(GameContext& context, Component::ParticleEmitter& emitter,
	                                    entt::registry& registry, entt::entity entity);

	void RenderPhysicsBodyComponent(GameContext& context, Component::PhysicsBody& physicsBody, entt::registry& registry,
	                                entt::entity entity);

	void RenderSpriteAnimationComponent(GameContext& context, Component::SpriteAnimation& spriteAnimation,
	                                    entt::registry& registry, entt::entity entity);

	void RenderLevelComponent(GameContext& context, Component::Level& level, entt::registry& registry,
	                          entt::entity entity);

	void RenderWorldComponent(GameContext& context, Component::World& world, entt::registry& registry,
	                          entt::entity entity);

	void RenderTileMapComponent(GameContext& context, Component::TileMap& tileMap, entt::registry& registry,
	                            entt::entity entity);

	void RenderWrenScriptComponent(GameContext& context, Component::WrenScript& script, entt::registry& registry,
	                               entt::entity entity);

	// Helper functions for rendering common data types
	bool RenderVec2(const char* label, glm::vec2& vec);
	bool RenderVec3(const char* label, glm::vec3& vec);
	bool RenderVec4(const char* label, glm::vec4& vec);
	bool RenderQuat(const char* label, glm::quat& quat);
	bool RenderColor(const char* label, Util::Color& color);

	HierarchyWindow* m_hierarchyWindow;
	UIHierarchyWindow* m_uiHierarchyWindow;
	PreviewWindow* m_previewWindow;
	std::unordered_map<std::string, ComponentRenderer> m_componentRenderers;
	InspectorDisplayType m_displayType;
	UI::UIElement* m_selectedUIElement;
	entt::entity m_selectedEntity;

	// "New Animation" form state, kept persistent across frames while the user fills it in
	char m_newAnimationNameBuffer[128] = "";
	int m_newAnimationStartFrame       = 0;
	int m_newAnimationEndFrame         = 1;
	float m_newAnimationTime           = 1.0f;
	bool m_newAnimationLoop            = true;

	// Currently selected cell in the TileMap grid editor, as row * width + col
	int m_selectedTileMapCell = -1;
};
}  // namespace Debug
}  // namespace Struktur
