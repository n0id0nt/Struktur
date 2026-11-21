#include "ProjectLauncher.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <filesystem>
#include <fstream>
#include <system_error>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Platform/Window.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Renderer/ImGuiRenderer.h"
#include "nlohmann/json.hpp"

// Project paths are kept as UTF-8 (tinyfiledialogs returns UTF-8 on Windows); std::filesystem::path's
// std::string constructor would read them in the ANSI code page instead, so go through char8_t explicitly.
static std::filesystem::path PathFromUtf8(const std::string& utf8Path)
{
	std::u8string utf8Characters(utf8Path.begin(), utf8Path.end());
	return std::filesystem::path(utf8Characters);
}

static std::string PathToUtf8(const std::filesystem::path& path)
{
	std::u8string utf8Characters = path.generic_u8string();
	return std::string(utf8Characters.begin(), utf8Characters.end());
}

// Absolute, forward-slashed, no trailing separator - one spelling per folder, so the list never holds duplicates
static std::string NormaliseProjectPath(const std::string& folderPath)
{
	std::error_code errorCode;
	std::filesystem::path absolutePath = std::filesystem::absolute(PathFromUtf8(folderPath), errorCode);
	if (errorCode)
	{
		absolutePath = PathFromUtf8(folderPath);
	}
	std::string normalisedPath = PathToUtf8(absolutePath.lexically_normal());
	while (normalisedPath.size() > 1 && normalisedPath.back() == '/' && normalisedPath[normalisedPath.size() - 2] != ':')
	{
		normalisedPath.pop_back();
	}
	return normalisedPath;
}

static bool IsProjectFolder(const std::string& folderPath)
{
	std::error_code errorCode;
	return std::filesystem::is_regular_file(PathFromUtf8(folderPath) / "Scripts" / "Main.wren", errorCode);
}

static bool IsSameFolder(const std::string& firstPath, const std::string& secondPath)
{
	std::error_code errorCode;
	bool equivalent = std::filesystem::equivalent(PathFromUtf8(firstPath), PathFromUtf8(secondPath), errorCode);
	if (errorCode)
	{
		return firstPath == secondPath;
	}
	return equivalent;
}

// The folder's own name - except a folder called "assets" (this repo's layout: <Project>/assets), which is
// named after its parent so the list doesn't fill up with identical "assets" entries.
static std::string ProjectNameFromPath(const std::string& folderPath)
{
	std::filesystem::path path = PathFromUtf8(folderPath);
	std::string folderName     = PathToUtf8(path.filename());
	if (folderName == "assets" && path.has_parent_path() && !path.parent_path().filename().empty())
	{
		return PathToUtf8(path.parent_path().filename());
	}
	return folderName.empty() ? folderPath : folderName;
}

std::string Struktur::Debug::ProjectLauncher::Run(GameContext& context)
{
	LoadProjectList();

	Platform::Window& window                 = context.GetWindow();
	Renderer::GraphicsDevice& graphicsDevice = context.GetGraphicsDevice();
	Renderer::ImGuiRenderer& imGuiRenderer   = context.GetImGuiRenderer();

	m_chosenProjectPath.clear();
	while (m_chosenProjectPath.empty())
	{
		window.PollEvents();
		if (window.ShouldClose())
		{
			return std::string();
		}
		if (window.WasResized())
		{
			graphicsDevice.Resize(window.GetWidth(), window.GetHeight());
		}

		graphicsDevice.BeginFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		RenderLauncher();
		ImGui::Render();
		imGuiRenderer.Render(ImGui::GetDrawData());
		graphicsDevice.EndFrame();
	}

	DEBUG_INFO("Opening project: " + m_chosenProjectPath);
	return m_chosenProjectPath;
}

void Struktur::Debug::ProjectLauncher::LoadProjectList()
{
	m_projects.clear();

	std::ifstream file(kProjectListFilePath);
	if (!file.is_open())
	{
		SeedDefaultProject();
		return;
	}

	nlohmann::json projectListJson = nlohmann::json::parse(file, nullptr, false);
	if (projectListJson.is_discarded() || !projectListJson.contains("projects") ||
	    !projectListJson["projects"].is_array())
	{
		DEBUG_WARNING(std::string("Couldn't read ") + kProjectListFilePath + ", starting a fresh project list");
		SeedDefaultProject();
		return;
	}

	for (const nlohmann::json& projectJson : projectListJson["projects"])
	{
		if (!projectJson.contains("path") || !projectJson["path"].is_string())
		{
			continue;
		}
		LauncherProject project;
		project.path = projectJson["path"].get<std::string>();
		project.name = projectJson.value("name", ProjectNameFromPath(project.path));
		m_projects.push_back(project);
	}
}

void Struktur::Debug::ProjectLauncher::SaveProjectList() const
{
	nlohmann::json projectArray = nlohmann::json::array();
	for (const LauncherProject& project : m_projects)
	{
		projectArray.push_back({{"name", project.name}, {"path", project.path}});
	}
	nlohmann::json projectListJson = {{"projects", projectArray}};

	std::ofstream file(kProjectListFilePath, std::ios::trunc);
	if (!file.is_open())
	{
		DEBUG_WARNING(std::string("Couldn't write ") + kProjectListFilePath);
		return;
	}
	file << projectListJson.dump(4);
}

void Struktur::Debug::ProjectLauncher::SeedDefaultProject()
{
	std::string defaultProjectPath = NormaliseProjectPath("assets");
	if (IsProjectFolder(defaultProjectPath))
	{
		m_projects.push_back({ProjectNameFromPath(defaultProjectPath), defaultProjectPath});
	}
	SaveProjectList();
}

void Struktur::Debug::ProjectLauncher::RenderLauncher()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags launcherFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
	                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));
	ImGui::Begin("Project Launcher", nullptr, launcherFlags);
	ImGui::PopStyleVar();

	ImGui::SetWindowFontScale(1.6f);
	ImGui::TextUnformatted("STRUKTUR");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::TextDisabled("Open a project, or add a folder that contains Scripts/Main.wren.");
	ImGui::Spacing();

	if (ImGui::Button("Add Project..."))
	{
		AddProjectFromFolderPicker();
	}
	if (!m_statusMessage.empty())
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.4f, 1.0f), "%s", m_statusMessage.c_str());
	}

	ImGui::Separator();
	RenderProjectTable();

	ImGui::End();
}

void Struktur::Debug::ProjectLauncher::RenderProjectTable()
{
	if (m_projects.empty())
	{
		ImGui::Spacing();
		ImGui::TextDisabled("No projects yet - use Add Project... to pick a project folder.");
		return;
	}

	ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY |
	                             ImGuiTableFlags_SizingStretchProp;
	if (!ImGui::BeginTable("Projects", 3, tableFlags))
	{
		return;
	}
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.0f);
	ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch, 3.0f);
	ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableHeadersRow();

	int projectToOpen = -1;
	for (int i = 0; i < (int)m_projects.size(); i++)
	{
		const LauncherProject& project = m_projects[i];
		bool projectAvailable          = IsProjectFolder(project.path);

		ImGui::PushID(i);
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		// Double-clicking anywhere on the row opens it too
		ImGuiSelectableFlags rowFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap |
		                                ImGuiSelectableFlags_AllowDoubleClick;
		if (ImGui::Selectable(project.name.c_str(), false, rowFlags) && projectAvailable &&
		    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			projectToOpen = i;
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::AlignTextToFramePadding();
		if (projectAvailable)
		{
			ImGui::TextUnformatted(project.path.c_str());
		}
		else
		{
			ImGui::TextDisabled("%s  (missing Scripts/Main.wren)", project.path.c_str());
		}

		ImGui::TableSetColumnIndex(2);
		ImGui::BeginDisabled(!projectAvailable);
		if (ImGui::Button("Open"))
		{
			projectToOpen = i;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Remove"))
		{
			m_pendingRemovalIndex = i;
			ImGui::OpenPopup("Remove Project?");
		}
		RenderRemoveConfirmation();

		ImGui::PopID();
	}
	ImGui::EndTable();

	if (projectToOpen >= 0)
	{
		OpenProject(projectToOpen);
	}
}

void Struktur::Debug::ProjectLauncher::RenderRemoveConfirmation()
{
	// Called inside each row's ID scope (where OpenPopup ran), so the popup's ID matches the row that opened it
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (!ImGui::BeginPopupModal("Remove Project?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}

	if (m_pendingRemovalIndex >= 0 && m_pendingRemovalIndex < (int)m_projects.size())
	{
		const LauncherProject& project = m_projects[m_pendingRemovalIndex];
		ImGui::Text("Remove \"%s\" from the project list?", project.name.c_str());
		ImGui::TextDisabled("%s", project.path.c_str());
		ImGui::TextUnformatted("The folder and its files are not deleted.");
		ImGui::Spacing();

		if (ImGui::Button("Remove", ImVec2(120.0f, 0.0f)))
		{
			RemoveProject(m_pendingRemovalIndex);
			m_pendingRemovalIndex = -1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
	}
	if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		m_pendingRemovalIndex = -1;
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}

void Struktur::Debug::ProjectLauncher::AddProjectFromFolderPicker()
{
	m_statusMessage.clear();

	std::string pickedFolder = FileSystem::OpenFolderDialog("Add Project Folder", FileSystem::GetWorkingDirectory());
	if (pickedFolder.empty())
	{
		return;
	}

	std::string projectPath = NormaliseProjectPath(pickedFolder);
	if (!IsProjectFolder(projectPath))
	{
		m_statusMessage = "Not a project: " + projectPath + " has no Scripts/Main.wren";
		return;
	}

	for (const LauncherProject& project : m_projects)
	{
		if (IsSameFolder(project.path, projectPath))
		{
			m_statusMessage = "Already in the list: " + project.name;
			return;
		}
	}

	m_projects.insert(m_projects.begin(), {ProjectNameFromPath(projectPath), projectPath});
	SaveProjectList();
}

void Struktur::Debug::ProjectLauncher::RemoveProject(int projectIndex)
{
	m_projects.erase(m_projects.begin() + projectIndex);
	m_statusMessage.clear();
	SaveProjectList();
}

void Struktur::Debug::ProjectLauncher::OpenProject(int projectIndex)
{
	LauncherProject project = m_projects[projectIndex];
	m_projects.erase(m_projects.begin() + projectIndex);
	m_projects.insert(m_projects.begin(), project);
	SaveProjectList();

	m_chosenProjectPath = project.path;
}
