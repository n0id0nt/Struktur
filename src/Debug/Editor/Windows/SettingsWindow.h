#pragma once

#include "EditorWindow.h"

namespace Struktur::Debug
{
class SettingsWindow : public EditorWindow
{
public:
	SettingsWindow()
	    : EditorWindow("Editor Settings")
	{
	}

	void Render(GameContext& context) override;

private:
	void RenderDebugRenderSettings(GameContext& context);
	void RenderViewportSettings(GameContext& context);
	void RenderHierarchySettings(GameContext& context);
	void RenderInspectorSettings(GameContext& context);
	void RenderGridSettings(GameContext& context);
	void RenderPerformanceSettings(GameContext& context);
	void RenderThemeSettings(GameContext& context);
};
}  // namespace Struktur::Debug
