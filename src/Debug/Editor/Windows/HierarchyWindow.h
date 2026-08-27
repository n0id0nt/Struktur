#pragma once

#include <entt/entt.hpp>

#include "EditorWindow.h"

namespace Struktur::Debug
{
class HierarchyWindow : public EditorWindow
{
public:
	HierarchyWindow()
	    : EditorWindow("Hierarchy"),
	      m_selectedEntity(entt::null)
	{
	}

	void Render(GameContext& context) override;

	// Get/Set selected entity
	entt::entity GetSelectedEntity() const
	{
		return m_selectedEntity;
	}
	void SetSelectedEntity(entt::entity entity)
	{
		m_selectedEntity = entity;
	}

private:
	// Render the entire scene graph starting from root entities
	void RenderSceneGraph(GameContext& context);

	// Recursively render a single entity node and its children
	void RenderEntityNode(GameContext& context, entt::entity entity);

	// Render context menu for an entity
	void RenderEntityContextMenu(GameContext& context, entt::entity entity);

	// Check if entity has children
	bool HasChildren(GameContext& context, entt::entity entity);

private:
	entt::entity m_selectedEntity;  // Currently selected entity in hierarchy
};
}  // namespace Struktur::Debug
