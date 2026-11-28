#pragma once

#include <bgfx/bgfx.h>
#include <imgui.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "Debug/Editor/EditorLayoutManager.h"
#include "Debug/Editor/EditorSettings.h"
#include "Debug/Editor/Windows/EditorWindow.h"
#ifdef STRUKTUR_WORLD2D
	#include "Debug/Editor/LevelViewer/LevelViewer.h"
#endif

namespace Struktur
{
class GameContext;

namespace Debug
{
class GameViewportWindow;

// The views debug drawing goes to: over the game's scene, and over the UI.
struct DebugViews
{
	bgfx::ViewId world  = 0;
	bgfx::ViewId screen = 0;
};

// What the toolbar's Play, Stop and Refresh do, set by the game.
struct PlayControls
{
	void (*start)(GameContext& context) = nullptr;
	void (*stop)(GameContext& context)  = nullptr;
	// Back to nothing loaded but with the scripts and physics up, for showing a scene without running the game (the
	// level viewer, so only games with the 2D world need it).
	void (*reset)(GameContext& context) = nullptr;
};

class Editor
{
public:
	Editor();
	~Editor() = default;

	// Initialise the editor
	void Initialise(GameContext& context);

	// Shutdown the editor
	void Shutdown(GameContext& context);

	// Update editor UI
	void Update(GameContext& context);

	// Create a new editor window
	template <typename T, typename... Args>
	T* CreateWindow(Args&&... args)
	{
		static_assert(std::is_base_of_v<EditorWindow, T>, "T must inherit from Struktur::Debug::EditorWindow");
		auto window                    = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr                         = window.get();
		m_windowMap[window->GetName()] = ptr;
		m_windows.push_back(std::move(window));
		return ptr;
	}

	// Get a window by name
	EditorWindow* GetWindow(const std::string& name);

	// Save the current docking layout and per-window open/closed state to disk
	void SaveEditorLayout();

	// Load the docking layout and per-window open/closed state from disk
	void LoadEditorLayout();

	// Get the game viewport window
	GameViewportWindow* GetGameViewport()
	{
		return m_gameViewport;
	}

	void SetPlayControls(const PlayControls& controls)
	{
		m_playControls = controls;
	}
	const PlayControls& GetPlayControls() const
	{
		return m_playControls;
	}

	void SetDebugViews(const DebugViews& views)
	{
		m_debugViews = views;
	}
	const DebugViews& GetDebugViews() const
	{
		return m_debugViews;
	}

	// Get editor settings
	EditorSettings& GetSettings()
	{
		return m_settings;
	}
	const EditorSettings& GetSettings() const
	{
		return m_settings;
	}

#ifdef STRUKTUR_WORLD2D
	// Shows LDtk worlds in the Game Viewport while the game is stopped
	LevelViewer& GetLevelViewer()
	{
		return m_levelViewer;
	}
#endif

private:
	// Render the complete editor layout
	void RenderEditorLayout(GameContext& context);

	// Render individual components
	void RenderMenuBar(GameContext& context);
	void RenderViewMenu();
	void RenderLayoutMenu();

	// Window management
	std::vector<std::unique_ptr<EditorWindow>> m_windows;
	std::unordered_map<std::string, EditorWindow*> m_windowMap;

	// Special windows (kept as raw pointers for quick access)
	GameViewportWindow* m_gameViewport;

	// Layout management
	EditorLayoutManager m_layoutManager;
	std::string m_currentLayout;

	// Editor settings
	EditorSettings m_settings;

	DebugViews m_debugViews;
	PlayControls m_playControls;

#ifdef STRUKTUR_WORLD2D
	LevelViewer m_levelViewer;
#endif

	// Editor state
	bool m_showDemoWindow;
};
}  // namespace Debug
}  // namespace Struktur
