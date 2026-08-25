#pragma once

#include "EditorWindow.h"

namespace Struktur::Debug
{
// Live view of the Wren-side game-flow state system (StateManager/BaseState - see assets/Scripts/States/).
// Shows the full active state stack (not just the root's current state - a state can nest its own StateManager
// for sub-states via BaseState.subStateManager, see GameWorldState), what params each level was entered with,
// each level's #!export-tagged fields (read/edit, same mechanism InspectorWindow uses for script components -
// see ExportedFieldRenderer.h/WrenExportedFields.h), and a "force transition" action for quick manual testing.
// Entirely pull-based: queries WrenStateManager fresh every Render(), no state of its own to keep in sync.
class StateWindow : public EditorWindow
{
public:
	StateWindow();

	void Render(GameContext& context) override;

private:
	void RenderToolbar(GameContext& context);

	// Selected index into the registered-state-names combo (toolbar "force transition" action).
	int m_selectedStateIndex = 0;
};
}  // namespace Struktur::Debug
