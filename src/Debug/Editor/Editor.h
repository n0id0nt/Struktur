#pragma once

#include <imgui.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "Debug/Editor/EditorLayoutManager.h"
#include "Debug/Editor/EditorSettings.h"
#include "Debug/Editor/Windows/EditorWindow.h"

namespace Struktur
{
class GameContext;

namespace Debug
{
class GameViewportWindow;

class Editor
{
   public:
	Editor();
	~Editor() = default;

	// Initialise the editor
	void Initialise(GameContext& context);

	// Shutdown the editor
	void Shutdown(GameContext& context);

	// Call before game update/render
	void BeginUpdateLoop(GameContext& context);

	// Call after game update/render
	void EndUpdateLoop(GameContext& context);

	// Update editor UI (call after EndUpdateLoop)
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

	// Get the game viewport window
	GameViewportWindow* GetGameViewport()
	{
		return m_gameViewport;
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

	// Editor state
	bool m_showDemoWindow;
};
}  // namespace Debug
}  // namespace Struktur
