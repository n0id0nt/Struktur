#include "HierarchyWindow.h"

#include "Engine/ECS/Component/Active.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/HierarchySystem.h"
#include "Engine/GameContext.h"

namespace Struktur::Debug
{
void HierarchyWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	ImGui::Text("Scene Objects:");
	ImGui::Separator();

	// Render the scene graph
	RenderSceneGraph(context);

	ImGui::Separator();

	// Toolbar buttons
	if (ImGui::Button("Add Entity"))
	{
		// TODO: Handle add entity
	}
	ImGui::SameLine();

	if (ImGui::Button("Delete"))
	{
		// TODO: Handle delete selected entity
		if (m_selectedEntity != entt::null)
		{
			// Delete logic here
		}
	}
	ImGui::SameLine();

	if (ImGui::Button("Deselect"))
	{
		m_selectedEntity = entt::null;
	}

	// Show selected entity info
	if (m_selectedEntity != entt::null)
	{
		ImGui::Separator();
		ImGui::Text("Selected: Entity %d", (int)m_selectedEntity);

		entt::registry& registry = context.GetRegistry();
		if (registry.valid(m_selectedEntity))
		{
			auto* identifier = registry.try_get<Component::Identifier>(m_selectedEntity);
			if (identifier)
			{
				ImGui::Text("Type: %s", identifier->type.c_str());
			}
		}
	}

	ImGui::End();
}

void HierarchyWindow::RenderSceneGraph(GameContext& context)
{
	entt::registry& registry = context.GetRegistry();

	// Get all entities with Identifier and Transform (basic entity components)
	auto view = registry.view<Component::Identifier, Component::Transform>();

	// Only render root entities (entities without parents)
	for (auto entity : view)
	{
		auto* parent = registry.try_get<Component::Parent>(entity);

		// Skip if this entity has a parent (it will be rendered as a child)
		if (parent && parent->entity != entt::null)
		{
			continue;
		}

		// Render this root entity and its children
		RenderEntityNode(context, entity);
	}
}

void HierarchyWindow::RenderEntityNode(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();

	// Safety check
	if (!registry.valid(entity))
	{
		return;
	}

	auto* identifier = registry.try_get<Component::Identifier>(entity);
	if (!identifier)
	{
		return;
	}

	// Determine node flags
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
	                           ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;

	// Check if this entity has children
	bool hasChildren = HasChildren(context, entity);

	// If no children, make it a leaf node (no arrow)
	if (!hasChildren)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	// Highlight if selected
	if (m_selectedEntity == entity)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	// Create unique ID for the tree node using entity handle
	void* nodeId = (void*)(intptr_t)entity;

	ImGui::PushID(nodeId);

	// Render the tree node
	bool nodeOpen = ImGui::TreeNodeEx(nodeId, flags, "%s", identifier->type.c_str());

	// Handle selection on click
	if (ImGui::IsItemClicked())
	{
		m_selectedEntity = entity;
	}

	// Render context menu
	RenderEntityContextMenu(context, entity);

	// Active toggle checkbox - lets the user show/hide (activate/deactivate) the object.
	// Right-aligned on the same row as the tree node.
	if (auto* active = registry.try_get<Component::Active>(entity))
	{
		bool isActive = active->activeState == Component::Active::ActiveState::Active;
		bool isInactiveParent = active->activeState == Component::Active::ActiveState::InactiveParent;

		float checkboxWidth = ImGui::GetFrameHeight();
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - checkboxWidth);

		// Disabled when inactive purely because an ancestor is inactive - can't override that from here
		ImGui::BeginDisabled(isInactiveParent);
		if (ImGui::Checkbox("##Active", &isActive))
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
	}

	// If node is open and has children, render them
	if (nodeOpen)
	{
		if (hasChildren)
		{
			auto* children = registry.try_get<Component::Children>(entity);
			if (children)
			{
				for (const auto& childEntity : children->entities)
				{
					RenderEntityNode(context, childEntity);
				}
			}
		}

		// IMPORTANT: Only pop if the node was opened
		ImGui::TreePop();
	}

	ImGui::PopID();
}

void HierarchyWindow::RenderEntityContextMenu(GameContext& context, entt::entity entity)
{
	if (ImGui::BeginPopupContextItem())
	{
		entt::registry& registry = context.GetRegistry();

		auto* identifier = registry.try_get<Component::Identifier>(entity);
		if (identifier)
		{
			ImGui::Text("Entity: %s", identifier->type.c_str());
			ImGui::Separator();
		}

		if (ImGui::MenuItem("Select"))
		{
			m_selectedEntity = entity;
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Add Child"))
		{
			// TODO: Create a new entity as a child of this one
		}

		if (ImGui::MenuItem("Duplicate"))
		{
			// TODO: Duplicate this entity
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Delete", "Del"))
		{
			// TODO: Delete this entity
			// Note: Be careful about deleting while iterating
		}

		ImGui::EndPopup();
	}
}

bool HierarchyWindow::HasChildren(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();
	auto* children           = registry.try_get<Component::Children>(entity);
	return children && !children->entities.empty();
}
}  // namespace Struktur::Debug
