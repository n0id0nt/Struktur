#include "SettingsWindow.h"

#include "Debug/Editor/Editor.h"
#include "Debug/Editor/EditorTheme.h"
#include "Engine/GameContext.h"

namespace Struktur::Debug
{
// Helper function for tooltips
static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void SettingsWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	ImGui::Text("Configure editor behavior and debug visualization");
	ImGui::Separator();

	// Use tabs for different setting categories
	if (ImGui::BeginTabBar("SettingsTabs"))
	{
		if (ImGui::BeginTabItem("Debug Rendering"))
		{
			RenderDebugRenderSettings(context);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Viewport"))
		{
			RenderViewportSettings(context);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Hierarchy"))
		{
			RenderHierarchySettings(context);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Inspector"))
		{
			RenderInspectorSettings(context);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Grid"))
		{
			RenderGridSettings(context);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Performance"))
		{
			RenderPerformanceSettings(context);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Theme"))
		{
			RenderThemeSettings(context);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::Separator();

	// Global actions
	if (ImGui::Button("Save Settings"))
	{
		context.GetEditor().GetSettings().SaveToFile("editor_settings.json");
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Settings"))
	{
		context.GetEditor().GetSettings().LoadFromFile("editor_settings.json");
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset to Defaults"))
	{
		// Reset all settings to defaults
		context.GetEditor().GetSettings() = EditorSettings();
	}

	ImGui::End();
}

void SettingsWindow::RenderDebugRenderSettings(GameContext& context)
{
	auto& settings = context.GetEditor().GetSettings().debugRender;

	ImGui::SeparatorText("In-Game Debug Visualization");

	if (ImGui::Checkbox("Show FPS Counter", &settings.showFPS))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}

	ImGui::Spacing();
	ImGui::SeparatorText("Physics Debug Rendering");

	if (ImGui::Checkbox("Show Physics Shapes", &settings.showPhysicsShapes))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}
	ImGui::SameLine();
	HelpMarker("Draw collision shape outlines");

	if (ImGui::Checkbox("Show Physics Bodies", &settings.showPhysicsBodies))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}
	ImGui::SameLine();
	HelpMarker("Draw rigid body information");

	if (ImGui::Checkbox("Show Physics Joints", &settings.showPhysicsJoints))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}
	ImGui::SameLine();
	HelpMarker("Draw joint connections");

	if (ImGui::Checkbox("Show Physics AABBs", &settings.showPhysicsAABBs))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}
	ImGui::SameLine();
	HelpMarker("Draw axis-aligned bounding boxes");

	if (ImGui::Checkbox("Show Contact Points", &settings.showPhysicsContactPoints))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}
	ImGui::SameLine();
	HelpMarker("Draw collision contact points");

	if (ImGui::SliderFloat("Physics Alpha", &settings.physicsShapeAlpha, 0.0f, 1.0f))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}

	ImGui::Spacing();
	ImGui::SeparatorText("Level & Entity Visualization");

	if (ImGui::Checkbox("Show Level Boundaries", &settings.showLevelBounds))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}

	if (ImGui::SliderFloat("Level Bounds Thickness", &settings.levelBoundsThickness, 1.0f, 5.0f))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}

	if (ImGui::Checkbox("Show Entity Gizmos", &settings.showEntityGizmos))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}
	ImGui::SameLine();
	HelpMarker("Show transform handles and icons");

	if (ImGui::Checkbox("Show Grid", &settings.showGrid))
	{
		context.GetEditor().GetSettings().NotifySettingChanged("debugRender");
	}
}

void SettingsWindow::RenderViewportSettings(GameContext& context)
{
	auto& settings = context.GetEditor().GetSettings().viewport;

	ImGui::SeparatorText("Viewport Display");

	ImGui::Checkbox("Show Statistics", &settings.showStats);

	ImGui::Checkbox("Lock Aspect Ratio", &settings.lockAspectRatio);

	if (settings.lockAspectRatio)
	{
		ImGui::SliderFloat("Target Aspect Ratio", &settings.targetAspectRatio, 1.0f, 3.0f, "%.2f");

		// Preset buttons
		if (ImGui::Button("16:9"))
		{
			settings.targetAspectRatio = 16.0f / 9.0f;
		}
		ImGui::SameLine();
		if (ImGui::Button("16:10"))
		{
			settings.targetAspectRatio = 16.0f / 10.0f;
		}
		ImGui::SameLine();
		if (ImGui::Button("4:3"))
		{
			settings.targetAspectRatio = 4.0f / 3.0f;
		}
		ImGui::SameLine();
		if (ImGui::Button("21:9"))
		{
			settings.targetAspectRatio = 21.0f / 9.0f;
		}
	}
}

void SettingsWindow::RenderHierarchySettings(GameContext& context)
{
	auto& settings = context.GetEditor().GetSettings().hierarchy;

	ImGui::SeparatorText("Display Options");

	ImGui::Checkbox("Show Entity IDs", &settings.showEntityIDs);
	ImGui::SameLine();
	HelpMarker("Display entity ID numbers in hierarchy");

	ImGui::Checkbox("Auto Expand New Entities", &settings.autoExpandNew);

	ImGui::Checkbox("Sort Alphabetically", &settings.sortAlphabetically);
}

void SettingsWindow::RenderInspectorSettings(GameContext& context)
{
	auto& settings = context.GetEditor().GetSettings().inspector;

	ImGui::SeparatorText("Inspector Behavior");

	ImGui::Checkbox("Show Advanced Properties", &settings.showAdvancedProperties);

	ImGui::Checkbox("Auto Expand Components", &settings.autoExpandComponents);

	ImGui::SliderFloat("Drag Speed", &settings.dragSpeed, 0.01f, 1.0f);
	ImGui::SameLine();
	HelpMarker("Speed of dragging float/int values");
}

void SettingsWindow::RenderGridSettings(GameContext& context)
{
	auto& settings = context.GetEditor().GetSettings().grid;

	ImGui::SeparatorText("Grid Display");

	ImGui::DragFloat("Grid Size", &settings.gridSize, 1.0f, 8.0f, 256.0f);

	ImGui::SliderFloat("Grid Opacity", &settings.gridOpacity, 0.0f, 1.0f);

	ImGui::Checkbox("Snap to Grid", &settings.snapToGrid);
	ImGui::SameLine();
	HelpMarker("Snap entity positions to grid when moving");
}

void SettingsWindow::RenderPerformanceSettings(GameContext& context)
{
	auto& settings = context.GetEditor().GetSettings().performance;

	ImGui::SeparatorText("Rendering");

#if defined(PLATFORM_WEB)
	if (ImGui::Checkbox("Enable VSync", &settings.enableVSync))
	{
		// Apply VSync setting immediately
		if (settings.enableVSync)
		{
			SetTargetFPS(settings.targetFPS);
		}
		else
		{
			SetTargetFPS(0);  // Unlimited
		}
	}

	if (ImGui::SliderInt("Target FPS", &settings.targetFPS, 30, 240))
	{
		if (settings.enableVSync)
		{
			SetTargetFPS(settings.targetFPS);
		}
	}
#else
	// Desktop's frame pacing comes from GraphicsDevice's BGFX_RESET_VSYNC, not raylib's SetTargetFPS - which
	// would hang here anyway, since raylib's core state (that function's target) is never initialised on this
	// path. Leaving the controls out entirely rather than showing settings that silently do nothing.
	ImGui::TextDisabled("VSync/target FPS controls aren't wired up on this platform yet.");
#endif

	ImGui::Spacing();
	ImGui::SeparatorText("Monitoring");

	ImGui::Checkbox("Show Performance Metrics", &settings.showPerformanceMetrics);

	if (settings.showPerformanceMetrics)
	{
		ImGui::Spacing();
		ImGui::Text("Current FPS: %d", GetFPS());
		ImGui::Text("Frame Time: %.3f ms", GetFrameTime() * 1000.0f);
	}
}

void SettingsWindow::RenderThemeSettings(GameContext& context)
{
	auto& settings = context.GetEditor().GetSettings().theme;

	ImGui::SeparatorText("Editor Theme");

	const char* themes[] = {"Dark", "Light", "Classic"};

	if (ImGui::Combo("Theme", &settings.currentTheme, themes, IM_ARRAYSIZE(themes)))
	{
		// Apply theme immediately
		switch (settings.currentTheme)
		{
			case 0:
				EditorTheme::ApplyDarkTheme();
				break;
			case 1:
				EditorTheme::ApplyLightTheme();
				break;
			case 2:
				EditorTheme::ApplyClassicTheme();
				break;
		}
	}

	ImGui::Spacing();

	// Quick preview
	ImGui::Text("Preview:");
	ImGui::Button("Button");
	ImGui::SameLine();
	static bool previewCheck = false;
	ImGui::Checkbox("Checkbox", &previewCheck);

	float value = 0.5f;
	ImGui::SliderFloat("Slider", &value, 0.0f, 1.0f);
}
}  // namespace Struktur::Debug
