#include "Editor.h"

#include <imgui_internal.h>

#include "Engine/Core/GameData.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/ImGuiRenderer.h"

// Include all window types
#include "Debug/Editor/EditorTheme.h"
#include "Debug/Editor/Windows/DebugInfoWindow.h"
#include "Debug/Editor/Windows/DialogueEditorWindow.h"
#include "Debug/Editor/Windows/FileExplorerWindow.h"
#include "Debug/Editor/Windows/GameViewportWindow.h"
#include "Debug/Editor/Windows/HierarchyWindow.h"
#include "Debug/Editor/Windows/InspectorWindow.h"
#include "Debug/Editor/Windows/LogWindow.h"
#include "Debug/Editor/Windows/PreviewWindow.h"
#include "Debug/Editor/Windows/ProfilerWindow.h"
#include "Debug/Editor/Windows/SettingsWindow.h"
#include "Debug/Editor/Windows/ToolbarWindow.h"
#include "Debug/Editor/Windows/UIHierarchyWindow.h"

namespace Struktur::Debug
{
Editor::Editor()
    : m_gameViewport(nullptr),
      m_currentLayout("Default"),
      m_showDemoWindow(false)
{
}

void Editor::Initialise(GameContext& context)
{
	// Create and register all editor windows

	// Game viewport (main rendering window)
	m_gameViewport = CreateWindow<GameViewportWindow>();

	// Hierarchy window (must be created before inspector)
	auto hierarchyPtr = CreateWindow<HierarchyWindow>();

	// Hierarchy window (must be created before inspector)
	auto uiHierarchyPtr = CreateWindow<UIHierarchyWindow>();

	// Debug info panel
	CreateWindow<DebugInfoWindow>(m_gameViewport);

	// Toolbar
	CreateWindow<ToolbarWindow>();

	// Settings window
	CreateWindow<SettingsWindow>();

	// Log window
	CreateWindow<LogWindow>();

	// Profiler window
	CreateWindow<ProfilerWindow>();

	// Preview window (must be created before inspector and file explorer)
	auto previewPtr = CreateWindow<PreviewWindow>();

	// File Explorer (depends on preview window)
	CreateWindow<FileExplorerWindow>(previewPtr, "assets");

	// Inspector window (depends on hierarchy)
	CreateWindow<InspectorWindow>(hierarchyPtr, uiHierarchyPtr, previewPtr);

	// Inspector window (depends on hierarchy)
	CreateWindow<DialogueEditorWindow>(previewPtr);

	// Route ImGui's own ini autosave/autoload through a single canonical file (also used by the explicit
	// Save/Load Layout menu actions), so docking state persists automatically across launches.
	ImGuiIO& io    = ImGui::GetIO();
	io.IniFilename = EditorLayoutManager::kDefaultLayoutFilePath;

	// Window open/closed state isn't part of ImGui's ini format (p_open is an app-owned bool) - restore it
	// explicitly from the last session, if any.
	std::unordered_map<std::string, bool> windowStates =
	    m_layoutManager.LoadWindowStates(EditorLayoutManager::kDefaultWindowStatesFilePath);
	for (auto& window : m_windows)
	{
		auto it = windowStates.find(window->GetName());
		if (it != windowStates.end())
		{
			window->SetOpen(it->second);
		}
	}

	// Initialise all windows
	for (auto& window : m_windows)
	{
		window->Initialise(context);
	}

	// Setup layouts
	m_layoutManager.CreateDefaultLayout();

	// Apply dark theme
	EditorTheme::ApplyDarkTheme();
}

void Editor::Shutdown(GameContext& context)
{
	// Persist docking layout and window open/closed state before the windows are torn down
	SaveEditorLayout();

	// Shutdown all windows
	for (auto& window : m_windows)
	{
		window->Shutdown(context);
	}

	m_windows.clear();
	m_windowMap.clear();
}

void Editor::Update(GameContext& context)
{
	// Setup dockspace and render all editor windows
	RenderEditorLayout(context);

	// Render demo window if enabled
	if (m_showDemoWindow)
	{
		ImGui::ShowDemoWindow(&m_showDemoWindow);
	}

	ImGui::Render();
	context.GetImGuiRenderer().Render(ImGui::GetDrawData());
}

EditorWindow* Editor::GetWindow(const std::string& name)
{
	auto it = m_windowMap.find(name);
	if (it != m_windowMap.end())
	{
		return it->second;
	}
	return nullptr;
}

void Editor::SaveEditorLayout()
{
	m_layoutManager.SaveLayout(EditorLayoutManager::kDefaultLayoutFilePath);

	std::unordered_map<std::string, bool> windowStates;
	for (auto& window : m_windows)
	{
		windowStates[window->GetName()] = window->IsOpen();
	}
	m_layoutManager.SaveWindowStates(windowStates, EditorLayoutManager::kDefaultWindowStatesFilePath);
}

void Editor::LoadEditorLayout()
{
	m_layoutManager.LoadLayout(EditorLayoutManager::kDefaultLayoutFilePath);

	std::unordered_map<std::string, bool> windowStates =
	    m_layoutManager.LoadWindowStates(EditorLayoutManager::kDefaultWindowStatesFilePath);
	for (auto& window : m_windows)
	{
		auto it = windowStates.find(window->GetName());
		if (it != windowStates.end())
		{
			window->SetOpen(it->second);
		}
	}
}

void Editor::RenderEditorLayout(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();

	// Create fullscreen dockspace window
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	window_flags |= ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
	ImGui::PopStyleVar(3);

	// Render menu bar
	RenderMenuBar(context);

	// Create dockspace
	ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	// Setup layout
	m_layoutManager.SetupLayout(dockspace_id, m_currentLayout, viewport->WorkSize);

	ImGui::End();

	// Render all registered windows
	for (auto& window : m_windows)
	{
		if (window->IsOpen())
		{
			window->Render(context);
		}
	}
}

void Editor::RenderMenuBar(GameContext& context)
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl+N"))
			{
				// Handle new file
			}
			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
				// Handle open file
			}
			if (ImGui::MenuItem("Save", "Ctrl+S"))
			{
				// Handle save
			}
			ImGui::Separator();

			if (ImGui::MenuItem("Save Layout"))
			{
				SaveEditorLayout();
			}
			if (ImGui::MenuItem("Load Layout"))
			{
				LoadEditorLayout();
			}

			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "Alt+F4"))
			{
				// Handle exit
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z"))
			{
				// Handle undo
			}
			if (ImGui::MenuItem("Redo", "Ctrl+Y"))
			{
				// Handle redo
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Settings", "Ctrl+,"))
			{
				// Open settings window
				auto* settingsWindow = GetWindow("Editor Settings");
				if (settingsWindow)
				{
					settingsWindow->SetOpen(true);
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			RenderViewMenu();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Layout"))
		{
			RenderLayoutMenu();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Theme"))
		{
			if (ImGui::MenuItem("Dark"))
			{
				EditorTheme::ApplyDarkTheme();
			}
			if (ImGui::MenuItem("Light"))
			{
				EditorTheme::ApplyLightTheme();
			}
			if (ImGui::MenuItem("Classic"))
			{
				EditorTheme::ApplyClassicTheme();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("ImGui Demo"))
			{
				m_showDemoWindow = !m_showDemoWindow;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("About"))
			{
				// Show about dialog
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void Editor::RenderViewMenu()
{
	// Toggle visibility of each window
	for (auto& window : m_windows)
	{
		bool isOpen = window->IsOpen();
		if (ImGui::MenuItem(window->GetName().c_str(), nullptr, &isOpen))
		{
			window->SetOpen(isOpen);
		}
	}
}

void Editor::RenderLayoutMenu()
{
	if (ImGui::MenuItem("Reset to Default"))
	{
		m_layoutManager.ResetToLayout("Default");
		m_currentLayout = "Default";
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Save Current Layout"))
	{
		SaveEditorLayout();
	}

	if (ImGui::MenuItem("Load Saved Layout"))
	{
		LoadEditorLayout();
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Reset All Layouts"))
	{
		m_layoutManager.ResetAllLayouts();
	}
}
}  // namespace Struktur::Debug
