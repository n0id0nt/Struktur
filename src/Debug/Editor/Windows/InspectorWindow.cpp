#include "InspectorWindow.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Debug/Editor/PreviewRenderers/PreviewHelpers.h"
#include "Debug/Editor/Windows/HierarchyWindow.h"
#include "Debug/Editor/Windows/PreviewWindow.h"
#include "Debug/Editor/Windows/UIHierarchyWindow.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Shader.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Game/RenderLayer.h"
#include "Engine/UI/UIElement.h"

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
	    [this](GameContext& context, Component::Transform& transform, entt::registry& registry,
	           entt::entity entity) { RenderLocalTransformComponent(context, transform, registry, entity); });

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
	RenderUIElementProperties(element);
}

void InspectorWindow::RenderUIElementHeader(UI::UIElement* element)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
	ImGui::Text("UI Element: %s", element->GetId().empty() ? "[Unnamed]" : element->GetId().c_str());
	ImGui::PopStyleColor();

	ImGui::Text("Type: UI Element");
}

void InspectorWindow::RenderUIElementProperties(UI::UIElement* element)
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

	// Transform
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushID("Transform");

		// Position
		glm::vec2 pos = element->GetPosition();
		if (RenderVec2("Position", pos))
		{
			// Note: UIElement doesn't have a direct SetPosition(vec2) method
			// You might need to calculate absolute/relative positions
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Position editing requires absolute/relative split");
		}

		// Size
		glm::vec2 size = element->GetSize();
		if (RenderVec2("Size", size))
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Size editing requires absolute/relative split");
		}

		// Bounds (read-only)
		Util::Math::Rect bounds = element->GetBounds();
		ImGui::Text("Bounds:");
		ImGui::Indent();
		ImGui::Text("X: %.1f, Y: %.1f", bounds.x, bounds.y);
		ImGui::Text("W: %.1f, H: %.1f", bounds.width, bounds.height);
		ImGui::Unindent();

		ImGui::PopID();
	}

	// Appearance
	if (ImGui::CollapsingHeader("Appearance"))
	{
		ImGui::PushID("Appearance");

		// Background color
		Util::Math::Color bgColor = element->GetBackgroundColor();
		if (RenderColor("Background Color", bgColor))
		{
			element->SetBackgroundColor(bgColor);
		}

		// Border color
		Util::Math::Color borderColor = element->GetBorderColor();
		if (RenderColor("Border Color", borderColor))
		{
			element->SetBorderColor(borderColor);
		}

		// Border width
		float borderWidth = element->GetBorderWidth();
		if (ImGui::DragFloat("Border Width", &borderWidth, 0.1f, 0.0f, 20.0f))
		{
			element->SetBorderWidth(borderWidth);
		}

		ImGui::PopID();
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

		const char* directions[] = {"Up", "Down", "Left", "Right"};
		for (int i = 0; i < 4; ++i)
		{
			auto* neighbor = element->GetNavigationNeighbor(static_cast<UI::NavigationDirection>(i));
			ImGui::Text("%s: %s", directions[i],
			            neighbor ? (neighbor->GetId().empty() ? "[Unnamed]" : neighbor->GetId().c_str()) : "[None]");
		}

		ImGui::PopID();
	}

	// Callbacks (read-only info)
	if (ImGui::CollapsingHeader("Callbacks"))
	{
		ImGui::PushID("Callbacks");
		ImGui::TextDisabled("Callback information is not directly accessible");
		ImGui::Text("Callbacks may be set via code");
		ImGui::PopID();
	}
}

// ====================================================================
// Component Renderers
// ====================================================================

void InspectorWindow::RenderLocalTransformComponent(GameContext& context, Component::Transform& transform,
                                                    entt::registry& registry, entt::entity entity)
{
	bool modified = false;

	glm::vec3 position = transform.localPosition;
	glm::vec3 scale     = transform.localScale;
	glm::quat rotation  = transform.localRotation;

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
	static const char* k_renderLayerNames[] = {"Background Far",    "Background Mid", "Entities",
	                                           "Background Overlay", "Foreground",     "UI"};
	int layerIndex = static_cast<int>(sprite.layer);
	if (ImGui::Combo("Render Layer", &layerIndex, k_renderLayerNames, IM_ARRAYSIZE(k_renderLayerNames)))
	{
		sprite.layer = static_cast<GameResource::RenderLayer>(layerIndex);
	}
	ImGui::DragFloat("Order In Layer", &sprite.orderInLayer, 1.0f, -1000.0f, 1000.0f);
	if (sprite.layer == GameResource::RenderLayer::Entities)
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

bool InspectorWindow::RenderColor(const char* label, Util::Math::Color& color)
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
