#include "LevelViewerWindow.h"

#include <algorithm>

#include "Debug/Editor/Editor.h"
#include "Debug/Editor/LevelViewer/LevelViewer.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/ECS/Component/World.h"
#include "Engine/GameContext.h"

namespace Struktur::Debug
{
namespace
{
constexpr const char* LEVELS_DIRECTORY = "Levels";
}

LevelViewerWindow::LevelViewerWindow(LevelViewer* levelViewer)
    : EditorWindow("Level Viewer"),
      m_levelViewer(levelViewer)
{
}

void LevelViewerWindow::Initialise(GameContext& context)
{
	RefreshWorldFiles();
}

void LevelViewerWindow::RefreshWorldFiles()
{
	m_worldFiles.clear();
	for (const std::string& name : FileSystem::ListDirectory(LEVELS_DIRECTORY))
	{
		if (name.size() > 5 && name.substr(name.size() - 5) == ".ldtk")
		{
			m_worldFiles.push_back(std::string(LEVELS_DIRECTORY) + "/" + name);
		}
	}
	std::sort(m_worldFiles.begin(), m_worldFiles.end());
	m_selectedWorldFile = std::clamp(m_selectedWorldFile, 0, std::max(0, (int)m_worldFiles.size() - 1));
}

void LevelViewerWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(320.0f, 420.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(m_name.c_str(), &m_isOpen))
	{
		ImGui::End();
		return;
	}

	const char* preview = m_worldFiles.empty() ? "(no .ldtk files)" : m_worldFiles[m_selectedWorldFile].c_str();
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::BeginCombo("##WorldFile", preview))
	{
		for (int i = 0; i < (int)m_worldFiles.size(); i++)
		{
			if (ImGui::Selectable(m_worldFiles[i].c_str(), i == m_selectedWorldFile))
			{
				m_selectedWorldFile = i;
			}
		}
		ImGui::EndCombo();
	}

	ImGui::BeginDisabled(m_worldFiles.empty());
	if (ImGui::Button("Open"))
	{
		m_levelViewer->Open(context, m_worldFiles[m_selectedWorldFile]);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!m_levelViewer->IsActive());
	if (ImGui::Button("Reload"))
	{
		m_levelViewer->Reload(context);
	}
	ImGui::SameLine();
	if (ImGui::Button("Frame All"))
	{
		m_levelViewer->FrameWorld(context);
	}
	ImGui::SameLine();
	if (ImGui::Button("Close"))
	{
		m_levelViewer->Close(context);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Rescan"))
	{
		RefreshWorldFiles();
	}

	ImGui::Separator();
	RenderLevelList(context);

	ImGui::End();
}

void LevelViewerWindow::RenderLevelList(GameContext& context)
{
	if (!m_levelViewer->IsActive())
	{
		ImGui::TextDisabled("No world open.");
		return;
	}

	auto* worldComponent = context.GetRegistry().try_get<Component::World>(m_levelViewer->GetWorldEntity());
	if (!worldComponent)
	{
		return;
	}
	const auto& levels = worldComponent->worldMap.levels;

	ImGui::Text("%s", m_levelViewer->GetFilePath().c_str());
	ImGui::TextDisabled("Right/middle drag: pan   Wheel: zoom   F: frame all");
	ImGui::TextDisabled("Click: select (Inspector)   Double-click a level: frame it");
	ImGui::TextDisabled("Ctrl+click: play from here");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Starts the game with the player at the clicked spot - used by a game state that loads\n"
		                  "this world at its LDtk positions (ExperimentState for SevenGods). Stop comes back here.");
	}

	LevelViewer::Options& options = m_levelViewer->GetOptions();
	ImGui::Checkbox("Auto reload", &options.autoReload);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Re-open the file when it is saved (e.g. from LDtk), keeping the camera and visibility.");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Animate previews", &options.animatePreviews);

	if (ImGui::CollapsingHeader("Levels", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (int i = 0; i < (int)levels.size(); i++)
		{
			const auto& level = levels[i];
			ImGui::PushID(level.Iid.c_str());
			bool visible = m_levelViewer->IsLevelVisible(level.Iid);
			if (ImGui::Checkbox("##Visible", &visible))
			{
				m_levelViewer->SetLevelVisible(context, level.Iid, visible);
			}
			ImGui::SameLine();
			if (ImGui::Selectable(level.identifier.c_str(), i == m_levelViewer->GetSelectedLevel(),
			                      ImGuiSelectableFlags_AllowDoubleClick))
			{
				m_levelViewer->SelectLevel(context, i);
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					m_levelViewer->FrameLevel(context, i);
				}
			}
			ImGui::SameLine();
			ImGui::TextDisabled("%dx%d", level.pxWid, level.pxHei);
			ImGui::PopID();
		}
	}

	if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// Every tile layer identifier in the world, in file order (levels share most of their layers).
		std::vector<std::string> layerIdentifiers;
		for (const auto& level : levels)
		{
			for (const auto& layer : level.layers)
			{
				if (layer.type != FileLoading::LevelParser::LayerType::ENTITIES &&
				    std::find(layerIdentifiers.begin(), layerIdentifiers.end(), layer.identifier) ==
				        layerIdentifiers.end())
				{
					layerIdentifiers.push_back(layer.identifier);
				}
			}
		}

		bool entitiesVisible = m_levelViewer->AreEntitiesVisible();
		if (ImGui::Checkbox("Entities", &entitiesVisible))
		{
			m_levelViewer->SetEntitiesVisible(context, entitiesVisible);
		}
		for (const std::string& layerIdentifier : layerIdentifiers)
		{
			bool visible = m_levelViewer->IsLayerVisible(layerIdentifier);
			if (ImGui::Checkbox(layerIdentifier.c_str(), &visible))
			{
				m_levelViewer->SetLayerVisible(context, layerIdentifier, visible);
			}
		}
	}

	if (ImGui::CollapsingHeader("Overlays"))
	{
		LevelViewer::OverlaySettings& overlay = m_levelViewer->GetOverlaySettings();
		ImGui::Checkbox("Level names", &overlay.levelNames);
		ImGui::Checkbox("Neighbour links", &overlay.neighbourLinks);
		ImGui::Checkbox("Placeholder labels", &overlay.placeholderLabels);
		ImGui::Checkbox("Level bounds", &context.GetEditor().GetSettings().debugRender.showLevelBounds);
	}
}
}  // namespace Struktur::Debug
