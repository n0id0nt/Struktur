#include "InspectorWindow.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Debug/Editor/ExportedFieldRenderer.h"
#include "Debug/Editor/PreviewRenderers/PreviewHelpers.h"
#include "Debug/Editor/Windows/HierarchyWindow.h"
#include "Debug/Editor/Windows/PreviewWindow.h"
#include "Debug/Editor/Windows/UIHierarchyWindow.h"
#include "Engine/Animation/SpriteAnimation.h"
#include "Engine/ECS/Component/Active.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/ParticleEmitter.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Shader.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/World.h"
#include "Engine/ECS/Component/WrenScript.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/HierarchySystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/FileLoading/LevelParser.h"
#include "Engine/GameContext.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Scripting/WrenScriptComponentRegistry.h"
#include "Engine/UI/UIBorder.h"
#include "Engine/UI/UIClip.h"
#include "Engine/UI/UIColor.h"
#include "Engine/UI/UIElement.h"
#include "Engine/UI/UILabel.h"
#include "Engine/UI/UIManager.h"
#include "Engine/UI/UINineSlice.h"
#include "Engine/UI/UIPanel.h"
#include "Engine/UI/UIRichLabel.h"
#include "Engine/UI/UIScroll.h"
#include "Engine/UI/UITexture.h"
#include "Engine/World/RenderLayer.h"

namespace Struktur::Debug
{
InspectorWindow::InspectorWindow(HierarchyWindow* hierarchyWindow, UIHierarchyWindow* uiHierarchyWindow,
                                 PreviewWindow* previewWindow)
    : EditorWindow("Inspector"),
      m_hierarchyWindow(hierarchyWindow),
      m_uiHierarchyWindow(uiHierarchyWindow),
      m_previewWindow(previewWindow),
      m_displayType(InspectorDisplayType::None),
      m_selectedUIElement(nullptr),
      m_selectedEntity(entt::null)
{
	// Register default component renderers
	RegisterDefaultRenderers();
}

void InspectorWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	// Check if we have a UI element selected
	UI::UIElement* selectedUIElement = m_uiHierarchyWindow->GetSelectedElement();
	if (selectedUIElement != m_selectedUIElement)
	{
		m_selectedUIElement = selectedUIElement;
		if (m_selectedUIElement)
		{
			m_displayType = InspectorDisplayType::UIElement;
			m_hierarchyWindow->SetSelectedEntity(entt::null);
		}
		else if (m_selectedEntity != entt::null)
		{
			m_displayType = InspectorDisplayType::Entity;
		}
		else
		{
			m_displayType = InspectorDisplayType::None;
		}
	}

	// Check if we have an entity selected
	entt::entity selectedEntity = m_hierarchyWindow->GetSelectedEntity();
	if (selectedEntity != m_selectedEntity)
	{
		m_selectedEntity = selectedEntity;
		if (m_selectedEntity != entt::null)
		{
			m_displayType = InspectorDisplayType::Entity;
			m_uiHierarchyWindow->SetSelectedElement(nullptr);
		}
		else if (m_selectedUIElement)
		{
			m_displayType = InspectorDisplayType::UIElement;
		}
		else
		{
			m_displayType = InspectorDisplayType::None;
		}
	}

	// Determine what to display based on most recent selection
	// UI elements take precedence if both are selected
	switch (m_displayType)
	{
		case InspectorDisplayType::UIElement:
		{
			RenderUIElementInspector(context, m_selectedUIElement);
		}
		case InspectorDisplayType::Entity:
		{
			RenderEntityInspector(context, m_selectedEntity);
		}
		case InspectorDisplayType::None:
		default:
			ImGui::Text("No selection.");
			ImGui::Text("Select an entity in the Hierarchy window");
			ImGui::Text("or a UI element in the UI Hierarchy window.");
	}

	ImGui::End();
}

void InspectorWindow::RegisterDefaultRenderers()
{
	// Register LocalTransform renderer
	RegisterComponentRenderer<Component::Transform>(
	    "LocalTransform",
	    [this](GameContext& context, Component::Transform& transform, entt::registry& registry, entt::entity entity)
	    { RenderLocalTransformComponent(context, transform, registry, entity); });

	// Register Sprite renderer
	RegisterComponentRenderer<Component::Sprite>("Sprite", [this](GameContext& context, Component::Sprite& sprite,
	                                                              entt::registry& registry, entt::entity entity)
	                                             { RenderSpriteComponent(context, sprite, registry, entity); });
}

// ====================================================================
// Entity Inspector Rendering
// ====================================================================

void InspectorWindow::RenderEntityInspector(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();

	if (!registry.valid(entity))
	{
		ImGui::Text("Selected entity is invalid.");
		return;
	}

	// Render entity header
	RenderEntityHeader(context, entity);

	ImGui::Separator();

	// Render all components
	RenderComponents(context, entity);
}

void InspectorWindow::RenderEntityHeader(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
	ImGui::Text("Entity: %d", (int)entity);
	ImGui::PopStyleColor();

	auto* identifier = registry.try_get<Component::Identifier>(entity);
	if (identifier)
	{
		ImGui::Text("Type: %s", identifier->type.c_str());
	}
}

void InspectorWindow::RenderComponents(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();

	// Render Active if exists
	if (auto* active = registry.try_get<Component::Active>(entity))
	{
		if (ImGui::CollapsingHeader("Active", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Active");
			RenderActiveComponent(context, *active, registry, entity);
			ImGui::PopID();
		}
	}

	// Render LocalTransform if exists
	if (auto* transform = registry.try_get<Component::Transform>(entity))
	{
		if (ImGui::CollapsingHeader("Local Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("LocalTransform");
			RenderLocalTransformComponent(context, *transform, registry, entity);
			ImGui::PopID();
		}
	}

	// Render Sprite if exists
	if (auto* sprite = registry.try_get<Component::Sprite>(entity))
	{
		if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Sprite");
			RenderSpriteComponent(context, *sprite, registry, entity);
			ImGui::PopID();
		}
	}

	// Render Shader if exists
	if (auto* shader = registry.try_get<Component::Shader>(entity))
	{
		if (ImGui::CollapsingHeader("Shader", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Shader");
			RenderShaderComponent(context, *shader, registry, entity);
			ImGui::PopID();
		}
	}

	// Render Camera if exists
	if (auto* camera = registry.try_get<Component::Camera>(entity))
	{
		if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Camera");
			RenderCameraComponent(context, *camera, registry, entity);
			ImGui::PopID();
		}
	}

	// Render ParticleEmitter if exists
	if (auto* particleEmitter = registry.try_get<Component::ParticleEmitter>(entity))
	{
		if (ImGui::CollapsingHeader("Particle Emitter", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("ParticleEmitter");
			RenderParticleEmitterComponent(context, *particleEmitter, registry, entity);
			ImGui::PopID();
		}
	}

	// Render PhysicsBody if exists
	if (auto* physicsBody = registry.try_get<Component::PhysicsBody>(entity))
	{
		if (ImGui::CollapsingHeader("Physics Body", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("PhysicsBody");
			RenderPhysicsBodyComponent(context, *physicsBody, registry, entity);
			ImGui::PopID();
		}
	}

	// Render SpriteAnimation if exists
	if (auto* spriteAnimation = registry.try_get<Component::SpriteAnimation>(entity))
	{
		if (ImGui::CollapsingHeader("Sprite Animation", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("SpriteAnimation");
			RenderSpriteAnimationComponent(context, *spriteAnimation, registry, entity);
			ImGui::PopID();
		}
	}

	// Render Level if exists
	if (auto* level = registry.try_get<Component::Level>(entity))
	{
		if (ImGui::CollapsingHeader("Level"))
		{
			ImGui::PushID("Level");
			RenderLevelComponent(context, *level, registry, entity);
			ImGui::PopID();
		}
	}

	// Render World if exists
	if (auto* world = registry.try_get<Component::World>(entity))
	{
		if (ImGui::CollapsingHeader("World"))
		{
			ImGui::PushID("World");
			RenderWorldComponent(context, *world, registry, entity);
			ImGui::PopID();
		}
	}

	// Render TileMap if exists
	if (auto* tileMap = registry.try_get<Component::TileMap>(entity))
	{
		if (ImGui::CollapsingHeader("Tile Map", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("TileMap");
			RenderTileMapComponent(context, *tileMap, registry, entity);
			ImGui::PopID();
		}
	}

	// Render WrenScript if exists
	if (auto* script = registry.try_get<Component::WrenScript>(entity))
	{
		if (ImGui::CollapsingHeader("Wren Script", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("WrenScript");
			RenderWrenScriptComponent(context, *script, registry, entity);
			ImGui::PopID();
		}
	}

	// Render Identifier if exists
	if (auto* identifier = registry.try_get<Component::Identifier>(entity))
	{
		if (ImGui::CollapsingHeader("Identifier"))
		{
			ImGui::PushID("Identifier");
			ImGui::Text("Type: %s", identifier->type.c_str());
			ImGui::PopID();
		}
	}

	// Render Parent if exists
	if (auto* parent = registry.try_get<Component::Parent>(entity))
	{
		if (ImGui::CollapsingHeader("Parent"))
		{
			ImGui::PushID("Parent");
			ImGui::Text("Parent Entity: %d", (int)parent->entity);
			ImGui::PopID();
		}
	}

	// Render Children if exists
	if (auto* children = registry.try_get<Component::Children>(entity))
	{
		if (ImGui::CollapsingHeader("Children"))
		{
			ImGui::PushID("Children");
			ImGui::Text("Child Count: %zu", children->entities.size());
			for (size_t i = 0; i < children->entities.size(); i++)
			{
				ImGui::Text("  Child %zu: %d", i, (int)children->entities[i]);
			}
			ImGui::PopID();
		}
	}
}

// ====================================================================
// UI Element Inspector Rendering
// ====================================================================

void InspectorWindow::RenderUIElementInspector(GameContext& context, UI::UIElement* element)
{
	if (!element)
	{
		ImGui::Text("Selected UI element is invalid.");
		return;
	}

	// Render UI element header
	RenderUIElementHeader(element);

	ImGui::Separator();

	// Render UI element properties
	RenderUIElementProperties(context, element);
}

void InspectorWindow::RenderUIElementHeader(UI::UIElement* element)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
	ImGui::Text("UI Element: %s", element->GetId().empty() ? "[Unnamed]" : element->GetId().c_str());
	ImGui::PopStyleColor();

	ImGui::Text("Type: UI Element");
}

void InspectorWindow::RenderUIElementProperties(GameContext& context, UI::UIElement* element)
{
	// Basic Properties
	if (ImGui::CollapsingHeader("Basic Properties", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushID("BasicProperties");

		// ID (editable)
		char idBuffer[256];
		strncpy(idBuffer, element->GetId().c_str(), sizeof(idBuffer) - 1);
		idBuffer[sizeof(idBuffer) - 1] = '\0';
		if (ImGui::InputText("ID", idBuffer, sizeof(idBuffer)))
		{
			element->SetId(idBuffer);
		}

		// State flags
		bool visible = element->IsVisible();
		if (ImGui::Checkbox("Visible", &visible))
		{
			element->SetVisible(visible);
		}

		bool enabled = element->IsEnabled();
		if (ImGui::Checkbox("Enabled", &enabled))
		{
			element->SetEnabled(enabled);
		}

		bool focusable = element->IsFocusable();
		if (ImGui::Checkbox("Focusable", &focusable))
		{
			element->SetFocusable(focusable);
		}

		ImGui::PopID();
	}

	// Transform - edits the raw absolute/relative components SetPosition/SetSize/SetAnchorPoint actually take
	// (see UIElement::GetAbsolutePosition's own comment for why the live, parent-walked GetPosition()/GetSize()
	// can't be edited directly: many different (absolute, relative) pairs can produce the same combined result
	// for a given parent size, so there's no way to reverse a live value back into the two). SetPosition/
	// SetSize/SetAnchorPoint all call UpdateBounds() internally (which recursively refreshes every descendant's
	// cached bounds and sets m_visualDirty - see UIElement.cpp), so editing here already gets the dirty-flag
	// propagation for free without this window needing to poke it directly.
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushID("Transform");

		glm::vec2 absPosition = element->GetAbsolutePosition();
		glm::vec2 relPosition = element->GetRelativePosition();
		bool positionChanged  = RenderVec2("Absolute Position", absPosition);
		positionChanged |= RenderVec2("Relative Position", relPosition);
		if (positionChanged)
		{
			element->SetPosition(absPosition, relPosition);
		}
		if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		{
			ImGui::SetTooltip("Relative Position is a fraction (0..1) of the parent's size, added to Absolute "
			                  "Position in pixels");
		}

		glm::vec2 absSize = element->GetAbsoluteSize();
		glm::vec2 relSize = element->GetRelativeSize();
		bool sizeChanged  = RenderVec2("Absolute Size", absSize);
		sizeChanged |= RenderVec2("Relative Size", relSize);
		if (sizeChanged)
		{
			element->SetSize(absSize, relSize);
		}

		glm::vec2 anchor = element->GetAnchorPoint();
		if (RenderVec2("Anchor Point", anchor))
		{
			element->SetAnchorPoint(anchor);
		}
		if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		{
			ImGui::SetTooltip("0..1 fraction of this element's own size subtracted from its position - (0.5, 0.5) "
			                  "centers the element on its position point instead of anchoring its top-left corner");
		}

		ImGui::Spacing();

		// Live, computed result of combining the above with the parent chain - read-only, shown for reference
		// while tuning the editable fields above.
		glm::vec2 livePosition  = element->GetPosition();
		glm::vec2 liveSize      = element->GetSize();
		Util::Math::Rect bounds = element->GetBounds();
		ImGui::TextDisabled("Computed Position: %.1f, %.1f", livePosition.x, livePosition.y);
		ImGui::TextDisabled("Computed Size: %.1f, %.1f", liveSize.x, liveSize.y);
		ImGui::TextDisabled("Bounds: X %.1f Y %.1f  W %.1f H %.1f", bounds.x, bounds.y, bounds.width, bounds.height);

		ImGui::PopID();
	}

	// Appearance - background/border live on whichever concrete type actually has them (UIPanel bundles all
	// three; UIColor/UITexture/UIBorder each have just their own one - see UIElement's class comment), not the
	// shared UIElement base, so which fields (if any) show up here depends on element's real type.
	if (auto* panel = dynamic_cast<UI::UIPanel*>(element))
	{
		if (ImGui::CollapsingHeader("Appearance"))
		{
			ImGui::PushID("Appearance");

			Util::Color bgColor = panel->GetBackgroundColor();
			if (RenderColor("Background Color", bgColor))
			{
				panel->SetBackgroundColor(bgColor);
			}

			Util::Color borderColor = panel->GetBorderColor();
			if (RenderColor("Border Color", borderColor))
			{
				panel->SetBorderColor(borderColor);
			}

			float borderWidth = panel->GetBorderWidth();
			if (ImGui::DragFloat("Border Width", &borderWidth, 0.1f, 0.0f, 20.0f))
			{
				panel->SetBorderWidth(borderWidth);
			}

			ImGui::PopID();
		}
	}
	else if (auto* color = dynamic_cast<UI::UIColor*>(element))
	{
		if (ImGui::CollapsingHeader("Appearance"))
		{
			ImGui::PushID("Appearance");

			Util::Color rectColor = color->GetColor();
			if (RenderColor("Color", rectColor))
			{
				color->SetColor(rectColor);
			}

			ImGui::PopID();
		}
	}
	else if (auto* border = dynamic_cast<UI::UIBorder*>(element))
	{
		if (ImGui::CollapsingHeader("Appearance"))
		{
			ImGui::PushID("Appearance");

			Util::Color borderColor = border->GetColor();
			if (RenderColor("Color", borderColor))
			{
				border->SetColor(borderColor);
			}

			float borderWidth = border->GetWidth();
			if (ImGui::DragFloat("Width", &borderWidth, 0.1f, 0.0f, 20.0f))
			{
				border->SetWidth(borderWidth);
			}

			ImGui::PopID();
		}
	}
	else if (auto* texture = dynamic_cast<UI::UITexture*>(element))
	{
		if (ImGui::CollapsingHeader("Appearance"))
		{
			ImGui::PushID("Appearance");
			RenderUITextureProperties(texture);
			ImGui::PopID();
		}
	}
	else if (auto* nineSlice = dynamic_cast<UI::UINineSlice*>(element))
	{
		if (ImGui::CollapsingHeader("Appearance"))
		{
			ImGui::PushID("Appearance");
			RenderUINineSliceProperties(nineSlice);
			ImGui::PopID();
		}
	}
	else if (auto* richLabel = dynamic_cast<UI::UIRichLabel*>(element))
	{
		if (ImGui::CollapsingHeader("Rich Text", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("RichText");
			RenderUIRichLabelProperties(richLabel);
			ImGui::PopID();
		}
	}
	else if (auto* label = dynamic_cast<UI::UILabel*>(element))
	{
		if (ImGui::CollapsingHeader("Text", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Text");
			RenderUILabelProperties(label);
			ImGui::PopID();
		}
	}
	// UIScroll before UIClip - UIScroll IS a UIClip (see UIScroll.h), so the UIClip check below would also
	// match a UIScroll instance if it ran first, hiding the scroll-specific section behind the generic one.
	else if (auto* scroll = dynamic_cast<UI::UIScroll*>(element))
	{
		if (ImGui::CollapsingHeader("Scroll", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Scroll");
			RenderUIScrollProperties(context, scroll);
			ImGui::PopID();
		}
	}
	else if (dynamic_cast<UI::UIClip*>(element))
	{
		if (ImGui::CollapsingHeader("Clip"))
		{
			ImGui::PushID("Clip");
			// Pure clipping boundary - masks children to this element's own Bounds (see Transform above), no
			// other configurable state (see UIClip's class comment). Nothing to expose here beyond what every
			// UIElement already has.
			ImGui::TextDisabled("No additional properties - masks children to Bounds above");
			ImGui::PopID();
		}
	}

	// Layout
	if (ImGui::CollapsingHeader("Layout"))
	{
		ImGui::PushID("Layout");

		// Z-Index
		int zIndex = element->GetZIndex();
		if (ImGui::DragInt("Z-Index", &zIndex, 1.0f, -100, 100))
		{
			element->SetZIndex(zIndex);
		}

		// Tab Index
		int tabIndex = element->GetTabIndex();
		if (ImGui::DragInt("Tab Index", &tabIndex, 1.0f, -1, 100))
		{
			element->SetTabIndex(tabIndex);
		}

		ImGui::PopID();
	}

	// Hierarchy
	if (ImGui::CollapsingHeader("Hierarchy"))
	{
		ImGui::PushID("Hierarchy");

		// Parent
		ImGui::Text("Parent: %s", element->GetParent() ? element->GetParent()->GetId().c_str() : "[None]");

		// Children
		const auto& children = element->GetChildren();
		ImGui::Text("Children: %zu", children.size());
		if (!children.empty())
		{
			ImGui::Indent();
			for (size_t i = 0; i < children.size(); ++i)
			{
				const auto& child = children[i];
				ImGui::Text("%zu: %s", i, child->GetId().empty() ? "[Unnamed]" : child->GetId().c_str());
			}
			ImGui::Unindent();
		}

		ImGui::PopID();
	}

	// Navigation
	if (ImGui::CollapsingHeader("Navigation"))
	{
		ImGui::PushID("Navigation");

		const char* directions[]     = {"Up", "Down", "Left", "Right"};
		const char* pickerLabels[]   = {"Up##NavPicker", "Down##NavPicker", "Left##NavPicker", "Right##NavPicker"};
		for (int i = 0; i < 4; ++i)
		{
			UI::NavigationDirection direction = static_cast<UI::NavigationDirection>(i);
			UI::UIElement* neighbor           = element->GetNavigationNeighbor(direction);
			if (RenderUIElementPicker(pickerLabels[i], context, element, neighbor))
			{
				element->SetNavigationNeighbor(direction, neighbor);
			}
			ImGui::SameLine();
			ImGui::TextDisabled("%s", directions[i]);
		}

		ImGui::PopID();
	}

	// Callbacks - deliberately read-only. A callback here (UIClickCallback etc., see UIElement.h) wraps a Wren
	// fiber/closure captured at script-authoring time (see Engine/Callback/WrenCallback.h) - there's no
	// serializable, editable representation of a closure to put in an ImGui panel, the same reason a
	// WrenScript component's methods aren't editable here either (only its #!export-tagged fields are, since
	// those are plain data, not code). Wiring a callback is a script/code-time decision, not a per-instance
	// tunable value, so it stays out of scope for this inspector by design rather than by omission.
	if (ImGui::CollapsingHeader("Callbacks"))
	{
		ImGui::PushID("Callbacks");
		ImGui::TextDisabled("Not editable here - callbacks are Wren closures captured in script, not data");
		ImGui::TextDisabled("(setOnClick/setOnFocus/setOnLoseFocus/setOnHover/setOnKeyPressed/setOnActivate)");
		ImGui::PopID();
	}
}

void InspectorWindow::RenderUITextureProperties(UI::UITexture* texture)
{
	// No in-editor texture/asset picker exists yet - matches every other texture field already in this
	// inspector (see RenderSpriteComponent/RenderTileMapComponent above, both status-only for the same reason).
	// Adding one would mean wiring a resource browser into this window, out of scope here.
	ImGui::Text("Texture: %s", texture->HasTexture() ? "Loaded" : "[None]");
	ImGui::TextDisabled("No asset picker in this build - set via script (Texture.load)");

	Util::Color tint = texture->GetTint();
	if (RenderColor("Tint", tint))
	{
		texture->SetTint(tint);
	}
}

void InspectorWindow::RenderUINineSliceProperties(UI::UINineSlice* nineSlice)
{
	ImGui::Text("Texture: %s", nineSlice->HasTexture() ? "Loaded" : "[None]");
	ImGui::TextDisabled("No asset picker in this build - set via script (Texture.load)");

	Util::Color tint = nineSlice->GetTint();
	if (RenderColor("Tint", tint))
	{
		nineSlice->SetTint(tint);
	}

	float borderLeft   = nineSlice->GetBorderLeft();
	float borderRight  = nineSlice->GetBorderRight();
	float borderTop    = nineSlice->GetBorderTop();
	float borderBottom = nineSlice->GetBorderBottom();
	bool borderChanged = false;
	borderChanged |= ImGui::DragFloat("Border Left", &borderLeft, 0.5f, 0.0f, 512.0f);
	borderChanged |= ImGui::DragFloat("Border Right", &borderRight, 0.5f, 0.0f, 512.0f);
	borderChanged |= ImGui::DragFloat("Border Top", &borderTop, 0.5f, 0.0f, 512.0f);
	borderChanged |= ImGui::DragFloat("Border Bottom", &borderBottom, 0.5f, 0.0f, 512.0f);
	if (borderChanged)
	{
		nineSlice->SetBorder(borderLeft, borderRight, borderTop, borderBottom);
	}
	ImGui::TextDisabled("Border insets are in SOURCE TEXTURE pixel space - corners keep this pixel size on");
	ImGui::TextDisabled("screen regardless of the element's own size (see UINineSlice.h)");
}

void InspectorWindow::RenderUIRichLabelProperties(UI::UIRichLabel* richLabel)
{
	// 4096 chars ought to cover any realistic UI label - if the live markup text is ever longer than that, this
	// truncates the buffer at construction (before any edit happens), matching the same char-buffer approach
	// already used for the ID field/WrenScript exported strings elsewhere in this window.
	std::string markup = richLabel->GetMarkupText();
	char buffer[4096];
	strncpy(buffer, markup.c_str(), sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';
	if (ImGui::InputTextMultiline("Markup Text", buffer, sizeof(buffer), ImVec2(0, 100)))
	{
		richLabel->SetMarkupText(buffer);
	}
	ImGui::TextDisabled("Tags: [b] [i] [color=#rrggbb] [icon=name] [wave] [shake] [pulse] [rainbow] [tornado] [fade]");

	Util::Color textColor = richLabel->GetTextColor();
	if (RenderColor("Base Text Color", textColor))
	{
		richLabel->SetTextColor(textColor);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Color a run starts with before any [color=] tag opens - and what an unbalanced "
		                  "closing [/color] falls back to");
	}

	float fontSize = richLabel->GetFontSize();
	if (ImGui::DragFloat("Font Size", &fontSize, 0.5f, 1.0f, 200.0f))
	{
		richLabel->SetFontSize(fontSize);
	}

	static const char* k_wrapNames[] = {"None", "Word Wrap", "Character Wrap"};
	int wrapIndex                    = static_cast<int>(richLabel->GetWordWrap());
	if (ImGui::Combo("Word Wrap", &wrapIndex, k_wrapNames, IM_ARRAYSIZE(k_wrapNames)))
	{
		richLabel->SetWordWrap(static_cast<UI::TextWrapping>(wrapIndex));
	}

	int visibleGlyphCount = richLabel->GetVisibleGlyphCount();
	if (ImGui::DragInt("Visible Glyph Count", &visibleGlyphCount, 1.0f, -1, 10000))
	{
		richLabel->SetVisibleGlyphCount(visibleGlyphCount);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("-1 shows everything - see SetVisibleGlyphCount for the typewriter/reveal use case");
	}

	bool useUnscaledTime = richLabel->GetUseUnscaledTime();
	if (ImGui::Checkbox("Animate With Unscaled Time", &useUnscaledTime))
	{
		richLabel->SetUseUnscaledTime(useUnscaledTime);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Off: [wave]/[shake]/[pulse]/etc. follow scaled game time (pauses/slows with gameplay). "
		                  "On: follows real unscaled time instead, so this label keeps animating through a pause "
		                  "or time-scale change - e.g. a main menu label rendered while gameplay time is stopped");
	}

	ImGui::Spacing();
	// Fonts/icon atlas: the same "no asset picker" gap as RenderUITextureProperties, just for
	// ResourcePtr<FontResource>/IconAtlas instead of a plain texture - status-only, set via script.
	ImGui::Text("Regular Font: %s", richLabel->GetFont() ? "Set" : "[None - required, always the final fallback]");
	ImGui::Text("Bold Font: %s", richLabel->GetBoldFont() ? "Set" : "[None - falls back to regular]");
	ImGui::Text("Italic Font: %s", richLabel->GetItalicFont() ? "Set" : "[None - synthetic shear fallback]");
	ImGui::Text("Bold Italic Font: %s", richLabel->GetBoldItalicFont() ? "Set" : "[None - falls back progressively]");
	ImGui::Text("Icon Atlas: %s", richLabel->HasIconAtlas() ? "Set" : "[None - [icon=] tags won't draw]");
	ImGui::TextDisabled("No asset picker in this build - set via script (Font.load / IconAtlas.new)");
}

void InspectorWindow::RenderUILabelProperties(UI::UILabel* label)
{
	std::string text = label->GetText();
	char buffer[4096];
	strncpy(buffer, text.c_str(), sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';
	if (ImGui::InputTextMultiline("Text", buffer, sizeof(buffer), ImVec2(0, 80)))
	{
		label->SetText(buffer);
	}

	Util::Color textColor = label->GetTextColor();
	if (RenderColor("Text Color", textColor))
	{
		label->SetTextColor(textColor);
	}

	float fontSize = label->GetFontSize();
	if (ImGui::DragFloat("Font Size", &fontSize, 0.5f, 1.0f, 200.0f))
	{
		label->SetFontSize(fontSize);
	}

	static const char* k_alignNames[] = {"Left", "Center", "Right", "Justify"};
	int alignIndex                    = static_cast<int>(label->GetAlignment());
	if (ImGui::Combo("Alignment", &alignIndex, k_alignNames, IM_ARRAYSIZE(k_alignNames)))
	{
		label->SetAlignment(static_cast<UI::TextAlignment>(alignIndex));
	}

	static const char* k_wrapNames[] = {"None", "Word Wrap", "Character Wrap"};
	int wrapIndex                    = static_cast<int>(label->GetWordWrap());
	if (ImGui::Combo("Word Wrap", &wrapIndex, k_wrapNames, IM_ARRAYSIZE(k_wrapNames)))
	{
		label->SetWordWrap(static_cast<UI::TextWrapping>(wrapIndex));
	}

	if (ImGui::Button("Set Bounding Box To Text"))
	{
		label->SetBoundingBoxToText();
	}

	ImGui::Spacing();
	ImGui::Text("Font: %s", label->GetFont() ? "Set" : "[None]");
	ImGui::TextDisabled("No asset picker in this build - set via script (Font.load)");
}

void InspectorWindow::RenderUIScrollProperties(GameContext& context, UI::UIScroll* scroll)
{
	bool verticalEnabled = scroll->IsVerticalScrollEnabled();
	if (ImGui::Checkbox("Vertical Scroll Enabled", &verticalEnabled))
	{
		scroll->SetVerticalScrollEnabled(verticalEnabled);
	}

	bool horizontalEnabled = scroll->IsHorizontalScrollEnabled();
	if (ImGui::Checkbox("Horizontal Scroll Enabled", &horizontalEnabled))
	{
		scroll->SetHorizontalScrollEnabled(horizontalEnabled);
	}

	glm::vec2 offset = scroll->GetScrollOffset();
	if (RenderVec2("Scroll Offset", offset))
	{
		scroll->SetScrollOffset(offset);
	}

	glm::vec2 contentSize = scroll->GetContentSize();
	ImGui::TextDisabled("Content Size (computed): %.1f, %.1f", contentSize.x, contentSize.y);

	glm::vec2 deadzone = scroll->GetFocusDeadzone();
	if (RenderVec2("Focus Deadzone", deadzone))
	{
		scroll->SetFocusDeadzone(deadzone);
	}

	glm::vec2 damping = scroll->GetFocusDamping();
	if (RenderVec2("Focus Damping", damping))
	{
		scroll->SetFocusDamping(damping);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Camera-style deadzone follow (see Component::Camera) - deadzone is the slack band "
		                  "around the viewport center, damping is the lerp rate used while catching up");
	}

	UI::UIElement* indicator = scroll->GetScrollIndicator();
	if (RenderUIElementPicker("Scroll Indicator", context, scroll, indicator))
	{
		scroll->SetScrollIndicator(indicator);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("A plain UIElement (typically a UIPanel) two-way synced as this scroll's thumb - see "
		                  "SetScrollIndicator");
	}
}

bool InspectorWindow::RenderUIElementPicker(const char* label, GameContext& context, UI::UIElement* excludeSelf,
                                            UI::UIElement*& value)
{
	std::vector<UI::UIElement*> candidates;
	candidates.push_back(nullptr);  // "[None]"
	for (const auto& root : context.GetUIManager().GetElements())
	{
		root->ForEachRecursive(
		    [&](UI::UIElement* elem)
		    {
			    if (elem != excludeSelf)
			    {
				    candidates.push_back(elem);
			    }
		    });
	}

	std::vector<std::string> labels;
	labels.reserve(candidates.size());
	int currentIndex = 0;
	for (int i = 0; i < (int)candidates.size(); ++i)
	{
		UI::UIElement* candidate = candidates[i];
		if (!candidate)
		{
			labels.push_back("[None]");
		}
		else if (!candidate->GetId().empty())
		{
			labels.push_back(candidate->GetId());
		}
		else
		{
			// No ID to show - listed by its position in this frame's walk instead so it's still selectable,
			// just not nameable. Fine for a debug tool; give it a real ID via SetId (Basic Properties above) if
			// you want it to show up meaningfully here.
			labels.push_back("[Unnamed " + std::to_string(i) + "]");
		}
		if (candidate == value)
		{
			currentIndex = i;
		}
	}

	std::vector<const char*> labelPtrs;
	labelPtrs.reserve(labels.size());
	for (const std::string& l : labels)
	{
		labelPtrs.push_back(l.c_str());
	}

	int selectedIndex = currentIndex;
	if (ImGui::Combo(label, &selectedIndex, labelPtrs.data(), (int)labelPtrs.size()))
	{
		value = candidates[selectedIndex];
		return true;
	}
	return false;
}

// ====================================================================
// Component Renderers
// ====================================================================

void InspectorWindow::RenderActiveComponent(GameContext& context, Component::Active& active, entt::registry& registry,
                                            entt::entity entity)
{
	bool isActive         = active.activeState == Component::Active::ActiveState::Active;
	bool isInactiveParent = active.activeState == Component::Active::ActiveState::InactiveParent;

	// Disabled when inactive purely because an ancestor is inactive - can't override that from here
	ImGui::BeginDisabled(isInactiveParent);
	if (ImGui::Checkbox("Active", &isActive))
	{
		auto& hierarchySystem = context.GetSystemManager().GetSystem<System::HierarchySystem>();
		if (isActive)
		{
			hierarchySystem.ActivevateEntity(context, entity);
		}
		else
		{
			hierarchySystem.DeactivevateEntity(context, entity);
		}
	}
	ImGui::EndDisabled();

	if (isInactiveParent)
	{
		ImGui::TextDisabled("Inactive because a parent is inactive");
	}
}

void InspectorWindow::RenderCameraComponent(GameContext& context, Component::Camera& camera, entt::registry& registry,
                                            entt::entity entity)
{
	ImGui::DragInt("Priority", &camera.cameraPriority);
	ImGui::DragFloat("Zoom", &camera.zoom, 0.01f, 0.01f, 100.0f);
	ImGui::DragFloat("Angle", &camera.angle, 0.5f);

	ImGui::Spacing();

	RenderVec2("Offset", camera.offset);
	RenderVec2("Dead Zone", camera.deadZone);
	RenderVec2("Damping", camera.damping);
	ImGui::Checkbox("Force Position", &camera.forcePosition);

	ImGui::Spacing();

	if (ImGui::TreeNode("Screen Shake"))
	{
		ImGui::DragFloat("Trauma", &camera.trauma, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Trauma Time", &camera.traumaTime, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Max Offset", &camera.maxOffset, 0.01f);
		ImGui::DragFloat("Max Angle", &camera.maxAngle, 0.01f);
		ImGui::DragFloat("Shake Amplitude", &camera.shakeAmplitude, 0.01f);

		ImGui::TreePop();
	}
}

void InspectorWindow::RenderPhysicsBodyComponent(GameContext& context, Component::PhysicsBody& physicsBody,
                                                 entt::registry& registry, entt::entity entity)
{
	if (!physicsBody.body)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No underlying Box2D body");
		return;
	}

	// Everything below reads/writes the live b2Body directly - this is the runtime physics state,
	// not just the ECS component, so edits here take effect immediately in the simulation.
	b2Body* body = physicsBody.body;

	Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
	const float ppm                     = physicsWorld.GetPixelsPerMeter();
	const float metersPerPixel          = 1.0f / ppm;

	static const char* k_bodyTypeNames[] = {"Static", "Kinematic", "Dynamic"};
	int bodyTypeIndex                    = static_cast<int>(body->GetType());
	if (ImGui::Combo("Body Type", &bodyTypeIndex, k_bodyTypeNames, IM_ARRAYSIZE(k_bodyTypeNames)))
	{
		body->SetType(static_cast<b2BodyType>(bodyTypeIndex));
	}

	ImGui::Spacing();

	b2Vec2 physPosition = body->GetPosition();
	glm::vec2 position(physPosition.x * ppm, physPosition.y * ppm);
	if (RenderVec2("Position", position))
	{
		body->SetTransform(b2Vec2(position.x * metersPerPixel, position.y * metersPerPixel), body->GetAngle());
	}

	float angleDegrees = glm::degrees(body->GetAngle());
	if (ImGui::DragFloat("Angle", &angleDegrees, 0.5f))
	{
		body->SetTransform(body->GetPosition(), glm::radians(angleDegrees));
	}

	ImGui::Spacing();

	b2Vec2 physVelocity = body->GetLinearVelocity();
	glm::vec2 linearVelocity(physVelocity.x, physVelocity.y);
	if (RenderVec2("Linear Velocity", linearVelocity))
	{
		body->SetLinearVelocity(b2Vec2(linearVelocity.x, linearVelocity.y));
	}

	float angularVelocity = body->GetAngularVelocity();
	if (ImGui::DragFloat("Angular Velocity", &angularVelocity, 0.1f))
	{
		body->SetAngularVelocity(angularVelocity);
	}

	ImGui::Spacing();

	float linearDamping = body->GetLinearDamping();
	if (ImGui::DragFloat("Linear Damping", &linearDamping, 0.01f, 0.0f, 10.0f))
	{
		body->SetLinearDamping(linearDamping);
	}

	float angularDamping = body->GetAngularDamping();
	if (ImGui::DragFloat("Angular Damping", &angularDamping, 0.01f, 0.0f, 10.0f))
	{
		body->SetAngularDamping(angularDamping);
	}

	float gravityScale = body->GetGravityScale();
	if (ImGui::DragFloat("Gravity Scale", &gravityScale, 0.01f))
	{
		body->SetGravityScale(gravityScale);
	}

	ImGui::Spacing();

	bool fixedRotation = body->IsFixedRotation();
	if (ImGui::Checkbox("Fixed Rotation", &fixedRotation))
	{
		body->SetFixedRotation(fixedRotation);
	}

	bool bullet = body->IsBullet();
	if (ImGui::Checkbox("Bullet", &bullet))
	{
		body->SetBullet(bullet);
	}

	bool sleepingAllowed = body->IsSleepingAllowed();
	if (ImGui::Checkbox("Sleeping Allowed", &sleepingAllowed))
	{
		body->SetSleepingAllowed(sleepingAllowed);
	}

	bool awake = body->IsAwake();
	if (ImGui::Checkbox("Awake", &awake))
	{
		body->SetAwake(awake);
	}

	bool enabled = body->IsEnabled();
	if (ImGui::Checkbox("Enabled", &enabled))
	{
		body->SetEnabled(enabled);
	}

	ImGui::Spacing();

	// --- Collision filter -------------------------------------------------------
	// b2Filter on the body's fixtures - a body can have more than one fixture, and they normally
	// share a single filter (PhysicsSystem::SetCollisionFilter, which the Wren binding calls, sets
	// them all), so a change here is applied to every fixture too. categoryBits = which layers this
	// body IS; maskBits = which layers it collides with. Two fixtures collide only when each one's
	// category bit appears in the other's mask. Layer names come from the CollisionLayers registry
	// (Engine/Physics/CollisionLayers.h; populated from script via physics.CollisionLayers).
	if (b2Fixture* firstFixture = body->GetFixtureList())
	{
		Physics::CollisionLayers& collisionLayers = context.GetCollisionLayers();
		b2Filter filter                           = firstFixture->GetFilterData();
		bool filterChanged                        = false;

		ImGui::SeparatorText("Collision Filter");

		bool hasNamedLayers = false;
		for (uint16_t bit = 0x0001; bit != 0; bit = static_cast<uint16_t>(bit << 1))
		{
			if (!collisionLayers.GetLayerName(bit).empty())
			{
				hasNamedLayers = true;
				break;
			}
		}

		if (hasNamedLayers)
		{
			auto renderLayerToggles = [&](const char* title, uint16& bits)
			{
				ImGui::TextDisabled("%s", title);
				ImGui::Indent();
				for (uint16_t bit = 0x0001; bit != 0; bit = static_cast<uint16_t>(bit << 1))
				{
					const std::string& layerName = collisionLayers.GetLayerName(bit);
					if (layerName.empty())
					{
						continue;
					}
					bool on = (bits & bit) != 0;
					if (ImGui::Checkbox(layerName.c_str(), &on))
					{
						bits = on ? static_cast<uint16>(bits | bit) : static_cast<uint16>(bits & ~bit);
						filterChanged = true;
					}
				}
				ImGui::Unindent();
			};

			ImGui::PushID("category");
			renderLayerToggles("Category (layers this body belongs to)", filter.categoryBits);
			ImGui::PopID();
			ImGui::PushID("mask");
			renderLayerToggles("Mask (layers this body collides with)", filter.maskBits);
			ImGui::PopID();
		}
		else
		{
			ImGui::TextDisabled("No named collision layers registered");
		}

		// Raw bitmask access - always available, so an unnamed bit or a body with no registered layers
		// can still be edited. Entered/shown as hex.
		auto renderRawBits = [&](const char* label, uint16& bits)
		{
			ImU16 value = bits;
			if (ImGui::InputScalar(label, ImGuiDataType_U16, &value, nullptr, nullptr, "%04X",
			                       ImGuiInputTextFlags_CharsHexadecimal))
			{
				bits          = value;
				filterChanged = true;
			}
		};
		renderRawBits("categoryBits", filter.categoryBits);
		renderRawBits("maskBits", filter.maskBits);

		int groupIndex = filter.groupIndex;
		if (ImGui::InputInt("groupIndex", &groupIndex))
		{
			filter.groupIndex = static_cast<int16>(groupIndex);
			filterChanged     = true;
		}

		if (filterChanged)
		{
			for (b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext())
			{
				fixture->SetFilterData(filter);
			}
		}
	}

	ImGui::Spacing();

	if (ImGui::TreeNode("Mass Data"))
	{
		ImGui::Text("Mass: %.3f kg", body->GetMass());
		ImGui::Text("Inertia: %.3f", body->GetInertia());
		ImGui::TreePop();
	}
}

void InspectorWindow::RenderSpriteAnimationComponent(GameContext& context, Component::SpriteAnimation& spriteAnimation,
                                                     entt::registry& registry, entt::entity entity)
{
	auto& animationSystem = context.GetSystemManager().GetSystem<System::AnimationSystem>();

	ImGui::Text("Current Animation: %s",
	            spriteAnimation.curAnimation.empty() ? "[None]" : spriteAnimation.curAnimation.c_str());

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Animations (%zu)", spriteAnimation.animations.size());

	// Editing the fields below writes straight into the component's animation map - the fields are
	// plain scalars so there's no need for a copy/apply step like Transform's quaternion conversion.
	for (auto& [name, animation] : spriteAnimation.animations)
	{
		ImGui::PushID(name.c_str());

		bool isCurrent = spriteAnimation.curAnimation == name;
		if (isCurrent)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
		}
		bool open = ImGui::TreeNodeEx(name.c_str(), isCurrent ? ImGuiTreeNodeFlags_DefaultOpen : 0, "%s%s",
		                              name.c_str(), isCurrent ? " (Current)" : "");
		if (isCurrent)
		{
			ImGui::PopStyleColor();
		}

		if (open)
		{
			int startFrame = static_cast<int>(animation.startFrame);
			if (ImGui::DragInt("Start Frame", &startFrame, 1.0f, 0, 10000))
			{
				animation.startFrame = static_cast<unsigned int>(startFrame);
			}

			int endFrame = static_cast<int>(animation.endFrame);
			if (ImGui::DragInt("End Frame", &endFrame, 1.0f, 0, 10000))
			{
				animation.endFrame = static_cast<unsigned int>(endFrame);
			}

			ImGui::DragFloat("Duration (s)", &animation.animationTime, 0.01f, 0.01f, 60.0f);
			ImGui::Checkbox("Loop", &animation.loop);

			if (ImGui::Button("Play"))
			{
				animationSystem.PlayAnimation(context, entity, name);
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	ImGui::Spacing();
	ImGui::Separator();

	if (ImGui::TreeNode("New Animation"))
	{
		ImGui::InputText("Name", m_newAnimationNameBuffer, sizeof(m_newAnimationNameBuffer));
		ImGui::DragInt("Start Frame##New", &m_newAnimationStartFrame, 1.0f, 0, 10000);
		ImGui::DragInt("End Frame##New", &m_newAnimationEndFrame, 1.0f, 0, 10000);
		ImGui::DragFloat("Duration (s)##New", &m_newAnimationTime, 0.01f, 0.01f, 60.0f);
		ImGui::Checkbox("Loop##New", &m_newAnimationLoop);

		bool nameEmpty = m_newAnimationNameBuffer[0] == '\0';
		bool nameTaken =
		    !nameEmpty && spriteAnimation.animations.find(m_newAnimationNameBuffer) != spriteAnimation.animations.end();

		ImGui::BeginDisabled(nameEmpty || nameTaken);
		if (ImGui::Button("Create Animation"))
		{
			Animation::SpriteAnimation newAnimation;
			newAnimation.startFrame    = static_cast<unsigned int>(m_newAnimationStartFrame);
			newAnimation.endFrame      = static_cast<unsigned int>(m_newAnimationEndFrame);
			newAnimation.animationTime = m_newAnimationTime;
			newAnimation.loop          = m_newAnimationLoop;

			animationSystem.AddAnimation(context, entity, m_newAnimationNameBuffer, newAnimation);

			m_newAnimationNameBuffer[0] = '\0';
			m_newAnimationStartFrame    = 0;
			m_newAnimationEndFrame      = 1;
			m_newAnimationTime          = 1.0f;
			m_newAnimationLoop          = true;
		}
		ImGui::EndDisabled();

		if (nameEmpty)
		{
			ImGui::TextDisabled("Enter a name to create an animation");
		}
		else if (nameTaken)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "An animation with this name already exists");
		}

		ImGui::TreePop();
	}
}

void InspectorWindow::RenderLevelComponent(GameContext& context, Component::Level& level, entt::registry& registry,
                                           entt::entity entity)
{
	// Sourced from the loaded level file (see World::Level::LoadLevelEntities) - not meaningful to edit here
	ImGui::Text("Index: %d", level.index);
	ImGui::Text("Iid: %s", level.Iid.c_str());
	ImGui::Text("Size: %d x %d px", level.width, level.height);
	ImGui::TextDisabled("Read-only - sourced from the loaded level file");
}

void InspectorWindow::RenderWorldComponent(GameContext& context, Component::World& world, entt::registry& registry,
                                           entt::entity entity)
{
	const auto& worldMap = world.worldMap;

	// Sourced from the loaded LDtk world file (see World::Level::CreateWorldEntity) - not meaningful to edit here
	ImGui::Text("Iid: %s", worldMap.Iid.c_str());
	ImGui::Text("File: %s", worldMap.filePath.c_str());
	ImGui::Text("Levels: %zu", worldMap.levels.size());
	ImGui::TextDisabled("Read-only - sourced from the loaded level file");

	ImGui::Spacing();

	static const char* k_layerTypeNames[] = {"Entities", "Int Grid", "Auto Layer", "Tiles"};

	for (size_t i = 0; i < worldMap.levels.size(); i++)
	{
		const auto& level = worldMap.levels[i];

		ImGui::PushID((int)i);
		if (ImGui::TreeNode(level.identifier.c_str(), "[%zu] %s", i, level.identifier.c_str()))
		{
			ImGui::Text("Iid: %s", level.Iid.c_str());
			ImGui::Text("World Position: (%d, %d)", level.worldX, level.worldY);
			ImGui::Text("Size: %d x %d px", level.pxWid, level.pxHei);
			ImGui::Text("Layers: %zu", level.layers.size());

			if (!level.neighbours.empty() && ImGui::TreeNode("Neighbours"))
			{
				for (const auto& neighbour : level.neighbours)
				{
					ImGui::BulletText("%s", neighbour.c_str());
				}
				ImGui::TreePop();
			}

			if (!level.layers.empty() && ImGui::TreeNode("Layers"))
			{
				for (const auto& layer : level.layers)
				{
					int typeIndex        = static_cast<int>(layer.type);
					const char* typeName = (typeIndex >= 0 && typeIndex < IM_ARRAYSIZE(k_layerTypeNames))
					                           ? k_layerTypeNames[typeIndex]
					                           : "Unknown";
					ImGui::BulletText("%s (%s)", layer.identifier.c_str(), typeName);
				}
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}

void InspectorWindow::RenderTileMapComponent(GameContext& context, Component::TileMap& tileMap,
                                             entt::registry& registry, entt::entity entity)
{
	if (tileMap.texture)
	{
		ImGui::Text("Texture: Loaded");
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Texture: None");
	}
	ImGui::Text("Grid: %d x %d cells (%d px tiles)", tileMap.width, tileMap.height, tileMap.tileSize);
	ImGui::Text("Visible Tiles: %zu", tileMap.gridTiles.size());
	ImGui::TextDisabled("The dense collision grid is baked into physics at load and isn't re-generated here");

	static const char* k_renderLayerNames[] = {"Background Far",     "Background Mid", "Entities",
	                                           "Background Overlay", "Foreground",     "UI"};
	int layerIndex                          = static_cast<int>(tileMap.layer);
	if (ImGui::Combo("Render Layer", &layerIndex, k_renderLayerNames, IM_ARRAYSIZE(k_renderLayerNames)))
	{
		tileMap.layer = static_cast<World::RenderLayer>(layerIndex);
	}
	ImGui::DragFloat("Order In Layer", &tileMap.orderInLayer, 1.0f, -1000.0f, 1000.0f);

	ImGui::Spacing();
	ImGui::Separator();

	const int cellCount = tileMap.width * tileMap.height;
	if (tileMap.width <= 0 || tileMap.height <= 0 || tileMap.tileSize <= 0)
	{
		ImGui::TextDisabled("No grid data");
		return;
	}
	if (cellCount > 4096)
	{
		ImGui::TextDisabled("Grid too large to edit here (%d cells)", cellCount);
		return;
	}

	// gridTiles only holds the tiles that are actually drawn (sparse), keyed by tile-local pixel position -
	// rebuild a cell -> gridTiles index lookup each frame rather than caching it, since edits below can
	// add/remove entries.
	std::unordered_map<int, int> cellToTileIndex;
	for (int i = 0; i < (int)tileMap.gridTiles.size(); i++)
	{
		const World::TileMap::GridTile& tile = tileMap.gridTiles[i];
		int col                              = (int)std::round(tile.position.x / tileMap.tileSize);
		int row                              = (int)std::round(tile.position.y / tileMap.tileSize);
		if (col >= 0 && col < tileMap.width && row >= 0 && row < tileMap.height)
		{
			cellToTileIndex[row * tileMap.width + col] = i;
		}
	}

	ImGui::Text("Click a cell to select it (blue = tile present)");
	ImGui::BeginChild("TileGrid", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

	const float cellButtonSize = 18.0f;
	for (int row = 0; row < tileMap.height; row++)
	{
		ImGui::PushID(row);
		for (int col = 0; col < tileMap.width; col++)
		{
			int cellIndex = row * tileMap.width + col;
			bool hasTile  = cellToTileIndex.find(cellIndex) != cellToTileIndex.end();
			bool selected = m_selectedTileMapCell == cellIndex;

			ImVec4 color = selected ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f)
			                        : (hasTile ? ImVec4(0.3f, 0.6f, 0.9f, 1.0f) : ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

			ImGui::PushID(col);
			ImGui::PushStyleColor(ImGuiCol_Button, color);
			if (ImGui::Button("##cell", ImVec2(cellButtonSize, cellButtonSize)))
			{
				m_selectedTileMapCell = cellIndex;
			}
			ImGui::PopStyleColor();
			ImGui::PopID();

			if (col < tileMap.width - 1)
			{
				ImGui::SameLine(0.0f, 2.0f);
			}
		}
		ImGui::PopID();
	}

	ImGui::EndChild();

	ImGui::Spacing();
	ImGui::Separator();

	if (m_selectedTileMapCell < 0 || m_selectedTileMapCell >= cellCount)
	{
		ImGui::TextDisabled("No cell selected");
		return;
	}

	int selectedRow = m_selectedTileMapCell / tileMap.width;
	int selectedCol = m_selectedTileMapCell % tileMap.width;
	ImGui::Text("Selected Cell: (%d, %d)", selectedCol, selectedRow);

	auto it = cellToTileIndex.find(m_selectedTileMapCell);
	if (it == cellToTileIndex.end())
	{
		ImGui::TextDisabled("Empty cell");
		if (ImGui::Button("Add Tile"))
		{
			World::TileMap::GridTile newTile;
			newTile.position       = glm::vec2(selectedCol * tileMap.tileSize, selectedRow * tileMap.tileSize);
			newTile.sourcePosition = glm::vec2(0.0f, 0.0f);
			newTile.flipBit        = World::TileMap::FlipBit::NONE;
			tileMap.gridTiles.push_back(newTile);
			tileMap.chunksBuilt = false;
		}
		return;
	}

	World::TileMap::GridTile& tile = tileMap.gridTiles[it->second];
	bool changed                   = false;

	int atlasCol = (int)std::round(tile.sourcePosition.x / tileMap.tileSize);
	int atlasRow = (int)std::round(tile.sourcePosition.y / tileMap.tileSize);
	if (ImGui::DragInt("Atlas Column", &atlasCol, 1.0f, 0, 1000))
	{
		changed = true;
	}
	if (ImGui::DragInt("Atlas Row", &atlasRow, 1.0f, 0, 1000))
	{
		changed = true;
	}
	if (changed)
	{
		tile.sourcePosition = glm::vec2(atlasCol * tileMap.tileSize, atlasRow * tileMap.tileSize);
	}

	static const char* k_flipNames[] = {"None", "Horizontal", "Vertical", "Both"};
	int flipIndex                    = static_cast<int>(tile.flipBit);
	if (ImGui::Combo("Flip", &flipIndex, k_flipNames, IM_ARRAYSIZE(k_flipNames)))
	{
		tile.flipBit = static_cast<World::TileMap::FlipBit>(flipIndex);
		changed      = true;
	}

	if (ImGui::Button("Clear Tile"))
	{
		tileMap.gridTiles.erase(tileMap.gridTiles.begin() + it->second);
		changed = true;
	}

	if (changed)
	{
		tileMap.chunksBuilt = false;
	}
}

void InspectorWindow::RenderWrenScriptComponent(GameContext& context, Component::WrenScript& script,
                                                entt::registry& registry, entt::entity entity)
{
	ImGui::Text("Class: %s", script.className.c_str());

#ifdef DEBUG
	if (!script.filePath.empty())
	{
		ImGui::TextDisabled("File: %s", script.filePath.c_str());
	}
#endif

	if (script.hasError)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", script.errorMessage.c_str());
	}
	else if (script.isInitialised)
	{
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Initialised");
	}
	else
	{
		ImGui::TextDisabled("Not yet initialised");
	}

	ImGui::Spacing();

	if (script.constructorArgs.empty())
	{
		ImGui::TextDisabled("No constructor arguments");
	}
	else if (ImGui::TreeNode("Constructor Args"))
	{
		for (const Wren::WrenItem& arg : script.constructorArgs)
		{
			switch (arg.type)
			{
				case WREN_TYPE_NUM:
					ImGui::Text("%s: %.4g", arg.identifier.c_str(), std::any_cast<double>(arg.value));
					break;
				case WREN_TYPE_BOOL:
					ImGui::Text("%s: %s", arg.identifier.c_str(), std::any_cast<bool>(arg.value) ? "true" : "false");
					break;
				case WREN_TYPE_STRING:
					ImGui::Text("%s: \"%s\"", arg.identifier.c_str(), std::any_cast<std::string>(arg.value).c_str());
					break;
				case WREN_TYPE_NULL:
					ImGui::Text("%s: null", arg.identifier.c_str());
					break;
				default:
					ImGui::Text("%s: <unsupported type>", arg.identifier.c_str());
					break;
			}
		}
		ImGui::TreePop();
	}

#ifdef DEBUG
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Exported Variables");

	const std::vector<Wren::WrenExportedField>& exportedFields =
	    context.GetWrenScriptComponentRegistry().GetExportedFields(context, script.className);

	if (exportedFields.empty())
	{
		ImGui::TextDisabled("None (tag a getter with #!export to expose it here)");
		return;
	}

	if (!script.isInitialised || script.hasError)
	{
		ImGui::TextDisabled("Available once the script initialises");
		return;
	}

	auto& scriptSystem = context.GetSystemManager().GetSystem<System::WrenScriptSystem>();

	for (const Wren::WrenExportedField& field : exportedFields)
	{
		ImGui::PushID(field.name.c_str());

		Wren::WrenItem value;
		if (!scriptSystem.GetExportedFieldValue(context, script, field.name, value))
		{
			ImGui::TextDisabled("%s: <unavailable>", field.name.c_str());
			ImGui::PopID();
			continue;
		}

		Wren::WrenItem newValue;
		if (RenderExportedField(field.name, value, field.hasSetter, newValue))
		{
			scriptSystem.SetExportedFieldValue(context, script, field.name, newValue);
		}

		if (!field.hasSetter)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(read-only)");
		}

		ImGui::PopID();
	}
#endif
}

void InspectorWindow::RenderLocalTransformComponent(GameContext& context, Component::Transform& transform,
                                                    entt::registry& registry, entt::entity entity)
{
	bool modified = false;

	glm::vec3 position = transform.localPosition;
	glm::vec3 scale    = transform.localScale;
	glm::quat rotation = transform.localRotation;

	// Position
	if (RenderVec3("Position", position))
	{
		modified = true;
	}

	// Rotation (as Euler angles for easier editing)
	glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(rotation));
	if (RenderVec3("Rotation", eulerAngles))
	{
		// Convert back to quaternion
		rotation = glm::quat(glm::radians(eulerAngles));
		modified = true;
	}

	// Scale
	if (RenderVec3("Scale", scale))
	{
		modified = true;
	}

	ImGui::Spacing();

	// Show matrix (read-only for now)
	if (ImGui::TreeNode("Transform Matrix"))
	{
		const glm::mat4& matrix = transform.localMatrix;
		ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0]);
		ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1]);
		ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2]);
		ImGui::Text("[ %.2f, %.2f, %.2f, %.2f ]", matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]);
		ImGui::TreePop();
	}

	ImGui::Spacing();

	// Reset buttons
	if (ImGui::Button("Reset Position"))
	{
		position = glm::vec3(0.0f);
		modified = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset Rotation"))
	{
		rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		modified = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset Scale"))
	{
		scale    = glm::vec3(1.0f);
		modified = true;
	}

	if (modified)
	{
		context.GetSystemManager().GetSystem<System::TransformSystem>().SetLocalTransform(context, entity, position,
		                                                                                  scale, rotation);
	}
}

void InspectorWindow::RenderSpriteComponent(GameContext& context, Component::Sprite& sprite, entt::registry& registry,
                                            entt::entity entity)
{
	// Preview button
	if (ImGui::Button("Preview Sprite"))
	{
		if (m_previewWindow)
		{
			PreviewSprite(m_previewWindow, &sprite, "Sprite Component");
		}
	}

	// Texture info
	if (sprite.texture)
	{
		ImGui::Text("Texture: Loaded");
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Texture: None");
	}

	ImGui::Spacing();

	// Color tint
	RenderColor("Color Tint", sprite.color);

	// Offset
	RenderVec2("Offset", sprite.offset);

	ImGui::Spacing();

	// Sprite sheet properties
	ImGui::Text("Sprite Sheet:");
	ImGui::DragInt("Columns", &sprite.columns, 1.0f, 1, 100);
	ImGui::DragInt("Rows", &sprite.rows, 1.0f, 1, 100);
	ImGui::DragInt("Index", &sprite.index, 1.0f, 0, sprite.columns * sprite.rows - 1);

	ImGui::Spacing();

	// Flipped
	ImGui::Checkbox("Flipped", &sprite.flipped);

	// Render layer + order
	static const char* k_renderLayerNames[] = {"Background Far",     "Background Mid", "Entities",
	                                           "Background Overlay", "Foreground",     "UI"};
	int layerIndex                          = static_cast<int>(sprite.layer);
	if (ImGui::Combo("Render Layer", &layerIndex, k_renderLayerNames, IM_ARRAYSIZE(k_renderLayerNames)))
	{
		sprite.layer = static_cast<World::RenderLayer>(layerIndex);
	}
	ImGui::DragFloat("Order In Layer", &sprite.orderInLayer, 1.0f, -1000.0f, 1000.0f);
	if (sprite.layer == World::RenderLayer::Entities)
	{
		ImGui::TextDisabled("(world Y is added automatically for y-sort on this layer)");
	}

	ImGui::Spacing();

	// Preview calculated values
	if (ImGui::TreeNode("Calculated Values"))
	{
		int totalFrames = sprite.columns * sprite.rows;
		ImGui::Text("Total Frames: %d", totalFrames);

		if (totalFrames > 0)
		{
			int row = sprite.index / sprite.columns;
			int col = sprite.index % sprite.columns;
			ImGui::Text("Current Frame: Row %d, Col %d", row, col);
		}

		ImGui::TreePop();
	}
}

void InspectorWindow::RenderParticleEmitterComponent(GameContext& context, Component::ParticleEmitter& emitter,
                                                      entt::registry& registry, entt::entity entity)
{
	// Texture info - status only, same "no asset picker in this build" convention as RenderSpriteComponent's
	// own texture field above.
	if (emitter.texture)
	{
		ImGui::Text("Texture: Loaded");
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Texture: None");
	}

	int aliveCount = 0;
	for (const auto& particle : emitter.particles)
	{
		if (particle.alive)
		{
			++aliveCount;
		}
	}
	ImGui::Text("Alive: %d / %d", aliveCount, emitter.maxParticles);

	ImGui::Spacing();

	// Sprite sheet properties - same fields/ranges as RenderSpriteComponent's own, since flipbook animation
	// (driving index from a particle's own age) reuses that exact math - see ParticleEmitter's own comment.
	ImGui::Text("Sprite Sheet:");
	ImGui::DragInt("Columns", &emitter.columns, 1.0f, 1, 100);
	ImGui::DragInt("Rows", &emitter.rows, 1.0f, 1, 100);

	ImGui::Spacing();

	ImGui::Text("Emission:");
	ImGui::Checkbox("Looping", &emitter.looping);
	ImGui::DragFloat("Emission Rate", &emitter.emissionRate, 0.5f, 0.0f, 1000.0f, "%.1f/sec");
	if (ImGui::DragInt("Burst Count", &emitter.burstCount, 1.0f, 0, 10000))
	{
		// A new burst count should fire again - matches wren_ParticleEmitterSetBurstCount's own behavior for
		// scripts (WrenGameObjectComponents.cpp), so editing this field live behaves the same way.
		emitter.hasBurst = false;
	}
	ImGui::DragFloat("Spawn Radius", &emitter.spawnRadius, 0.5f, 0.0f, 1000.0f);
	ImGui::DragInt("Max Particles", &emitter.maxParticles, 1.0f, 1, 10000);

	ImGui::Spacing();

	ImGui::Text("Motion:");
	RenderVec2("Velocity Min", emitter.velocityMin);
	RenderVec2("Velocity Max", emitter.velocityMax);
	RenderVec2("Acceleration", emitter.acceleration);
	ImGui::DragFloat("Rotation Speed Min", &emitter.rotationSpeedMin, 0.01f, -20.0f, 20.0f, "%.2f rad/s");
	ImGui::DragFloat("Rotation Speed Max", &emitter.rotationSpeedMax, 0.01f, -20.0f, 20.0f, "%.2f rad/s");

	ImGui::Spacing();

	ImGui::Text("Lifetime:");
	ImGui::DragFloat("Lifetime Min", &emitter.lifetimeMin, 0.05f, 0.01f, 60.0f, "%.2fs");
	ImGui::DragFloat("Lifetime Max", &emitter.lifetimeMax, 0.05f, 0.01f, 60.0f, "%.2fs");

	ImGui::Spacing();

	ImGui::Text("Appearance Over Lifetime:");
	RenderColor("Start Color", emitter.startColor);
	RenderColor("End Color", emitter.endColor);
	ImGui::DragFloat("Start Scale", &emitter.startScale, 0.05f, 0.0f, 100.0f);
	ImGui::DragFloat("End Scale", &emitter.endScale, 0.05f, 0.0f, 100.0f);
	ImGui::Checkbox("Additive Blend", &emitter.additive);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Order-independent - the natural default for sparks/fire/glow. Off gives normal alpha "
		                  "blending, better for smoke/dust (see WorldRenderer's own additive-blend comment).");
	}

	ImGui::Spacing();

	// Render layer + order - same fields/ranges as RenderSpriteComponent's own.
	static const char* k_renderLayerNames[] = {"Background Far",     "Background Mid", "Entities",
	                                           "Background Overlay", "Foreground",     "UI"};
	int layerIndex                          = static_cast<int>(emitter.layer);
	if (ImGui::Combo("Render Layer", &layerIndex, k_renderLayerNames, IM_ARRAYSIZE(k_renderLayerNames)))
	{
		emitter.layer = static_cast<World::RenderLayer>(layerIndex);
	}
	ImGui::DragFloat("Order In Layer", &emitter.orderInLayer, 1.0f, -1000.0f, 1000.0f);
}

void InspectorWindow::RenderShaderComponent(GameContext& context, Component::Shader& shader, entt::registry& registry,
                                            entt::entity entity)
{
	// Preview button
	if (ImGui::Button("Preview Shader"))
	{
		if (m_previewWindow)
		{
			PreviewShader(m_previewWindow, &shader, "Shader Component");
		}
	}

	ImGui::Separator();

	ImGui::Text("Shader ID: %d", shader.shader->shader.idx);
	ImGui::Separator();

	// Float uniforms
	if (ImGui::TreeNode("Float Uniforms"))
	{
		if (shader.floatUniforms.empty())
		{
			ImGui::TextDisabled("No float uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.floatUniforms)
			{
				ImGui::PushID(name.c_str());
				ImGui::DragFloat(name.c_str(), &value, 0.01f);
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Int uniforms
	if (ImGui::TreeNode("Int Uniforms"))
	{
		if (shader.intUniforms.empty())
		{
			ImGui::TextDisabled("No int uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.intUniforms)
			{
				ImGui::PushID(name.c_str());
				ImGui::DragInt(name.c_str(), &value);
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Vec2 uniforms
	if (ImGui::TreeNode("Vector2 Uniforms"))
	{
		if (shader.vec2Uniforms.empty())
		{
			ImGui::TextDisabled("No vec2 uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.vec2Uniforms)
			{
				ImGui::PushID(name.c_str());
				float v[2] = {value.x, value.y};
				if (ImGui::DragFloat2(name.c_str(), v, 0.01f))
				{
					value.x = v[0];
					value.y = v[1];
				}
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Vec3 uniforms
	if (ImGui::TreeNode("Vector3 Uniforms"))
	{
		if (shader.vec3Uniforms.empty())
		{
			ImGui::TextDisabled("No vec3 uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.vec3Uniforms)
			{
				ImGui::PushID(name.c_str());
				float v[3] = {value.x, value.y, value.z};
				if (ImGui::DragFloat3(name.c_str(), v, 0.01f))
				{
					value.x = v[0];
					value.y = v[1];
					value.z = v[2];
				}
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Vec4 uniforms
	if (ImGui::TreeNode("Vector4 Uniforms"))
	{
		if (shader.vec4Uniforms.empty())
		{
			ImGui::TextDisabled("No vec4 uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.vec4Uniforms)
			{
				ImGui::PushID(name.c_str());
				float v[4] = {value.x, value.y, value.z, value.w};
				if (ImGui::DragFloat4(name.c_str(), v, 0.01f))
				{
					value.x = v[0];
					value.y = v[1];
					value.z = v[2];
					value.w = v[3];
				}
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Matrix uniforms (read-only for now)
	if (ImGui::TreeNode("Matrix Uniforms"))
	{
		if (shader.matrixUniforms.empty())
		{
			ImGui::TextDisabled("No matrix uniforms");
		}
	}
}

// ====================================================================
// Helper functions for rendering common data types
// ====================================================================

bool InspectorWindow::RenderVec2(const char* label, glm::vec2& vec)
{
	float values[2] = {vec.x, vec.y};
	bool modified   = ImGui::DragFloat2(label, values, 0.1f);
	if (modified)
	{
		vec.x = values[0];
		vec.y = values[1];
	}
	return modified;
}

bool InspectorWindow::RenderVec3(const char* label, glm::vec3& vec)
{
	float values[3] = {vec.x, vec.y, vec.z};
	bool modified   = ImGui::DragFloat3(label, values, 0.1f);
	if (modified)
	{
		vec.x = values[0];
		vec.y = values[1];
		vec.z = values[2];
	}
	return modified;
}

bool InspectorWindow::RenderVec4(const char* label, glm::vec4& vec)
{
	float values[4] = {vec.x, vec.y, vec.z, vec.w};
	bool modified   = ImGui::DragFloat4(label, values, 0.1f);
	if (modified)
	{
		vec.x = values[0];
		vec.y = values[1];
		vec.z = values[2];
		vec.w = values[3];
	}
	return modified;
}

bool InspectorWindow::RenderQuat(const char* label, glm::quat& quat)
{
	// Display as Euler angles for easier editing
	glm::vec3 euler = glm::degrees(glm::eulerAngles(quat));
	float values[3] = {euler.x, euler.y, euler.z};
	bool modified   = ImGui::DragFloat3(label, values, 0.5f);
	if (modified)
	{
		euler.x = values[0];
		euler.y = values[1];
		euler.z = values[2];
		quat    = glm::quat(glm::radians(euler));
	}
	return modified;
}

bool InspectorWindow::RenderColor(const char* label, Util::Color& color)
{
	float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};

	bool modified = ImGui::ColorEdit4(label, col);

	if (modified)
	{
		color.r = (unsigned char)(col[0] * 255.0f);
		color.g = (unsigned char)(col[1] * 255.0f);
		color.b = (unsigned char)(col[2] * 255.0f);
		color.a = (unsigned char)(col[3] * 255.0f);
	}

	return modified;
}

}  // namespace Struktur::Debug
