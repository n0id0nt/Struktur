#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace Struktur::Debug
{
// Centralized settings system accessible through the Editor
class EditorSettings
{
   public:
	EditorSettings()  = default;
	~EditorSettings() = default;

	// Debug rendering settings
	struct DebugRenderSettings
	{
		bool showFPS                  = true;
		bool audio                    = true;
		bool showPhysicsShapes        = true;
		bool showPhysicsBodies        = true;
		bool showPhysicsJoints        = true;
		bool showPhysicsAABBs         = false;
		bool showPhysicsContactPoints = false;
		bool showLevelBounds          = true;
		bool showEntityGizmos         = false;
		bool showGrid                 = false;
		bool playingGame              = false;
		bool pausedGame               = false;

		float physicsShapeAlpha    = 0.7f;
		float levelBoundsThickness = 2.0f;
	} debugRender;

	// Editor viewport settings
	struct ViewportSettings
	{
		bool showStats          = true;
		bool lockAspectRatio    = true;
		float targetAspectRatio = 16.0f / 9.0f;
	} viewport;

	// Hierarchy settings
	struct HierarchySettings
	{
		bool showEntityIDs      = false;
		bool autoExpandNew      = true;
		bool sortAlphabetically = false;
	} hierarchy;

	// Inspector settings
	struct InspectorSettings
	{
		bool showAdvancedProperties = false;
		bool autoExpandComponents   = true;
		float dragSpeed             = 0.1f;
	} inspector;

	// Grid settings
	struct GridSettings
	{
		float gridSize    = 64.0f;
		float gridOpacity = 0.3f;
		bool snapToGrid   = false;
	} grid;

	// Performance settings
	struct PerformanceSettings
	{
		bool enableVSync            = true;
		int targetFPS               = 60;
		bool showPerformanceMetrics = false;
	} performance;

	// Theme settings
	struct ThemeSettings
	{
		int currentTheme = 0;  // 0=Dark, 1=Light, 2=Classic
	} theme;

	// Callbacks for when settings change
	void RegisterOnChangeCallback(const std::string& settingCategory, std::function<void()> callback)
	{
		m_changeCallbacks[settingCategory] = callback;
	}

	void NotifySettingChanged(const std::string& settingCategory)
	{
		auto it = m_changeCallbacks.find(settingCategory);
		if (it != m_changeCallbacks.end())
		{
			it->second();
		}
	}

	// Save/Load settings (future implementation)
	void SaveToFile(const std::string& filename);
	void LoadFromFile(const std::string& filename);

   private:
	std::unordered_map<std::string, std::function<void()>> m_changeCallbacks;
};
}  // namespace Struktur::Debug
