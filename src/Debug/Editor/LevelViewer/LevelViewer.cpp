#include "LevelViewer.h"

#include <imgui.h>

#include <format>
#include <limits>

#include "Debug/Assertions.h"
#include "Debug/Editor/Editor.h"
#include "Debug/Editor/Windows/GameViewportWindow.h"
#include "Debug/Editor/Windows/HierarchyWindow.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/LevelEntityInstance.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/World.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/HierarchySystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/Game.h"
#include "Engine/GameContext.h"
#include "Engine/World/Level.h"
#include "nlohmann/json.hpp"

namespace Struktur::Debug
{
namespace
{
constexpr ImU32 LEVEL_NAME_COLOR     = IM_COL32(255, 203, 0, 255);
constexpr ImU32 LABEL_BACKGROUND     = IM_COL32(0, 0, 0, 170);
constexpr ImU32 SELECTED_COLOR       = IM_COL32(102, 191, 255, 255);
constexpr ImU32 HOVERED_COLOR        = IM_COL32(255, 255, 255, 200);
constexpr ImU32 NEIGHBOUR_LINK_COLOR = IM_COL32(255, 161, 0, 160);
constexpr float LABEL_PADDING        = 3.f;
// How often the open file's modification time is checked (seconds).
constexpr float FILE_POLL_INTERVAL = 0.5f;

// The "Name" field if the instance has one (most do - NPC, Item, ...), otherwise its LDtk identifier.
std::string GetDisplayName(const Component::LevelEntityInstance& instance)
{
	for (const auto& field : instance.fields)
	{
		if (field.identifier == "Name" && field.type == WrenType::WREN_TYPE_STRING)
		{
			return std::any_cast<std::string>(field.value);
		}
	}
	return instance.identifier;
}

void DrawLabel(ImDrawList* drawList, const ImVec2& position, ImU32 color, const std::string& text)
{
	ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
	ImVec2 max(position.x + textSize.x + LABEL_PADDING * 2, position.y + textSize.y + LABEL_PADDING * 2);
	drawList->AddRectFilled(position, max, LABEL_BACKGROUND);
	drawList->AddText(ImVec2(position.x + LABEL_PADDING, position.y + LABEL_PADDING), color, text.c_str());
}
}  // namespace

void LevelViewer::Open(GameContext& context, const std::string& filePath)
{
	auto& debugSettings       = context.GetEditor().GetSettings().debugRender;
	debugSettings.playingGame = false;
	debugSettings.pausedGame  = false;
	m_reopenAfterPlay         = false;

	// Same reset Play does, minus starting the game state - leaves a clean scene with the engine systems up.
	ClearGameSystems(context);
	StartGameSystems(context);

	// The registry was just cleared, so any selection points at a dead entity.
	SelectEntity(context, entt::null);
	if (filePath != m_filePath)
	{
		m_selectedLevel = -1;
	}
	m_hoveredEntity = entt::null;
	m_hoveredLevel  = -1;

	m_filePath    = filePath;
	m_worldEntity = World::Level::CreateWorldEntity(context, filePath);

	auto modTime     = FileSystem::GetModificationTime(filePath);
	m_fileModTime    = modTime.success ? modTime.value : 0;
	m_pendingModTime = 0;
	m_timeSincePoll  = 0.f;

	World::Level::LevelLoadOptions options;
	options.createPhysics = false;
	options.entityMode    = World::Level::EntityMode::PREVIEW;

	auto& worldComponent = context.GetRegistry().get<Component::World>(m_worldEntity);
	int levelCount       = static_cast<int>(worldComponent.worldMap.levels.size());
	for (int levelIndex = 0; levelIndex < levelCount; levelIndex++)
	{
		World::Level::LoadLevelEntities(context, m_worldEntity, levelIndex, options);
	}
	ApplyVisibility(context);

	DEBUG_INFO("[Level Viewer] Opened %s (%d levels)", filePath.c_str(), levelCount);

	auto savedView = m_savedViews.find(m_filePath);
	if (savedView != m_savedViews.end())
	{
		m_camera.SetView(savedView->second);
		m_camera.Apply(context);
	}
	else
	{
		FrameWorld(context);
	}
}

void LevelViewer::Close(GameContext& context)
{
	m_filePath.clear();
	m_worldEntity     = entt::null;
	m_reopenAfterPlay = false;
	m_selectedLevel   = -1;
	m_hoveredEntity   = entt::null;
	m_hoveredLevel    = -1;
	SelectEntity(context, entt::null);
	ClearGameSystems(context);
}

void LevelViewer::Reload(GameContext& context)
{
	if (m_filePath.empty())
	{
		return;
	}
	Open(context, m_filePath);
}

void LevelViewer::OnPlayStarted()
{
	m_reopenAfterPlay = IsActive();
	m_worldEntity     = entt::null;
	m_hoveredEntity   = entt::null;
}

void LevelViewer::OnPlayStopped(GameContext& context)
{
	if (m_reopenAfterPlay)
	{
		Reload(context);
	}
}

void LevelViewer::Update(GameContext& context)
{
	if (!IsActive())
	{
		return;
	}

	GameViewportWindow* viewport = context.GetEditor().GetGameViewport();
	if (viewport)
	{
		m_camera.HandleInput(context, *viewport);

		ImGuiIO& io = ImGui::GetIO();
		if ((viewport->IsViewportHovered() || viewport->IsViewportFocused()) && !io.WantTextInput &&
		    ImGui::IsKeyPressed(ImGuiKey_F, false))
		{
			FrameWorld(context);
		}
	}

	m_camera.Apply(context);
	m_savedViews[m_filePath] = m_camera.GetView();

	if (m_options.animatePreviews)
	{
		context.GetSystemManager().GetSystem<System::AnimationSystem>().Update(context);
	}

	// A reload rebuilt the scene - leave picking until next frame, when the viewport has drawn the new one.
	if (m_options.autoReload && PollForFileChanges(context))
	{
		return;
	}

	if (!viewport)
	{
		return;
	}
	UpdateHover(context, *viewport);

	// The size-preset buttons float over the image - a click on one of those isn't a pick.
	if (m_isMouseOverView && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
	{
		if (ImGui::GetIO().KeyCtrl)
		{
			PlayFrom(context, m_mouseWorldPos);
			return;
		}
		if (m_hoveredEntity != entt::null)
		{
			m_selectedLevel = m_hoveredLevel;
			SelectEntity(context, m_hoveredEntity);
		}
		else
		{
			SelectLevel(context, m_hoveredLevel);
		}
	}

	if (m_hoveredEntity != entt::null)
	{
		const auto& instance = context.GetRegistry().get<Component::LevelEntityInstance>(m_hoveredEntity);
		ImGui::SetTooltip("%s (%s)", GetDisplayName(instance).c_str(), instance.identifier.c_str());
	}
}

void LevelViewer::PlayFrom(GameContext& context, const glm::vec2& worldPos)
{
	Core::GameData& gameData     = context.GetGameData();
	gameData.debugSpawnWorldFile = m_filePath;
	gameData.debugSpawnPosition  = worldPos;
	auto& debugSettings          = context.GetEditor().GetSettings().debugRender;
	debugSettings.playingGame    = true;
	debugSettings.pausedGame     = false;
	DEBUG_INFO("[Level Viewer] Play from %s (%.0f, %.0f)", m_filePath.c_str(), worldPos.x, worldPos.y);
	OnPlayStarted();
	StartDebugGame(context);
}

bool LevelViewer::PollForFileChanges(GameContext& context)
{
	m_timeSincePoll += context.GetTimeSystem().unscaledDelta;
	if (m_timeSincePoll < FILE_POLL_INTERVAL)
	{
		return false;
	}
	m_timeSincePoll = 0.f;

	auto modTime = FileSystem::GetModificationTime(m_filePath);
	if (!modTime.success || modTime.value == m_fileModTime)
	{
		m_pendingModTime = 0;
		return false;
	}
	// First sighting of this change - give the writer a poll to finish.
	if (modTime.value != m_pendingModTime)
	{
		m_pendingModTime = modTime.value;
		return false;
	}
	// LevelParser asserts on a file it can't read and nlohmann throws on bad JSON, so only reload a complete file.
	auto contents = FileSystem::ReadString(m_filePath);
	if (!contents.success || !nlohmann::json::accept(contents.value))
	{
		return false;
	}
	DEBUG_INFO("[Level Viewer] %s changed on disk - reloading", m_filePath.c_str());
	Reload(context);
	return true;
}

void LevelViewer::UpdateHover(GameContext& context, const GameViewportWindow& viewport)
{
	m_hoveredEntity = entt::null;
	m_hoveredLevel  = -1;

	glm::vec2 mouseGamePos = viewport.GetGameMousePosition(context);
	m_isMouseOverView      = mouseGamePos.x >= 0.f;
	if (!m_isMouseOverView)
	{
		return;
	}
	m_mouseWorldPos = context.GetCamera().ScreenPosToWorldPos(mouseGamePos);
	m_hoveredLevel  = FindLevelAt(context, m_mouseWorldPos);

	// Smallest rect under the cursor wins, so an item on top of a big placeholder can still be picked.
	entt::registry& registry                 = context.GetRegistry();
	System::TransformSystem& transformSystem = context.GetSystemManager().GetSystem<System::TransformSystem>();
	float smallestArea                       = std::numeric_limits<float>::max();
	auto view = registry.view<Component::LevelEntityInstance, Component::Transform>(entt::exclude<System::Inactive>);
	for (auto [entity, instance, transform] : view.each())
	{
		glm::vec2 min = glm::vec2(transformSystem.GetWorldPosition(context, entity)) + instance.offset;
		glm::vec2 max = min + instance.size;
		float area    = instance.size.x * instance.size.y;
		if (m_mouseWorldPos.x >= min.x && m_mouseWorldPos.y >= min.y && m_mouseWorldPos.x < max.x &&
		    m_mouseWorldPos.y < max.y && area < smallestArea)
		{
			smallestArea    = area;
			m_hoveredEntity = entity;
		}
	}
}

void LevelViewer::RenderViewportOverlay(GameContext& context, ImDrawList* drawList, const glm::vec2& imagePos,
                                        const glm::vec2& imageSize)
{
	entt::registry& registry = context.GetRegistry();
	auto* worldComponent     = registry.try_get<Component::World>(m_worldEntity);
	if (!worldComponent)
	{
		return;
	}

	System::TransformSystem& transformSystem = context.GetSystemManager().GetSystem<System::TransformSystem>();
	Core::GameData& gameData                 = context.GetGameData();
	World::Camera& camera                    = context.GetCamera();
	glm::vec2 gameToScreen = imageSize / glm::vec2((float)gameData.gameWidth, (float)gameData.gameHeight);
	auto toScreen          = [&](const glm::vec2& worldPos)
	{
		glm::vec2 screen = imagePos + camera.WorldPosToScreenPos(worldPos) * gameToScreen;
		return ImVec2(screen.x, screen.y);
	};

	drawList->PushClipRect(ImVec2(imagePos.x, imagePos.y), ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y),
	                       true);

	const auto& levels = worldComponent->worldMap.levels;
	if (m_overlay.neighbourLinks)
	{
		auto centre = [](const FileLoading::LevelParser::Level& level)
		{ return glm::vec2(level.worldX + level.pxWid / 2.f, level.worldY + level.pxHei / 2.f); };
		for (int i = 0; i < (int)levels.size(); i++)
		{
			for (const auto& neighbour : levels[i].neighbours)
			{
				int j = FileLoading::LevelParser::FindLevelIndexByIid(worldComponent->worldMap, neighbour.levelIid);
				// Neighbour lists are symmetric - draw each link once.
				if (j <= i || !IsLevelVisible(levels[i].Iid) || !IsLevelVisible(levels[j].Iid))
				{
					continue;
				}
				drawList->AddLine(toScreen(centre(levels[i])), toScreen(centre(levels[j])), NEIGHBOUR_LINK_COLOR,
				                  2.f);
			}
		}
	}

	for (int i = 0; i < (int)levels.size(); i++)
	{
		const auto& level = levels[i];
		if (!IsLevelVisible(level.Iid))
		{
			continue;
		}
		ImVec2 min = toScreen(glm::vec2(level.worldX, level.worldY));
		if (i == m_selectedLevel)
		{
			drawList->AddRect(min, toScreen(glm::vec2(level.worldX + level.pxWid, level.worldY + level.pxHei)),
			                  SELECTED_COLOR, 0.f, 0, 3.f);
		}
		if (m_overlay.levelNames)
		{
			DrawLabel(drawList, ImVec2(min.x + 4.f, min.y + 4.f),
			          i == m_selectedLevel ? SELECTED_COLOR : LEVEL_NAME_COLOR, level.identifier);
		}
	}

	entt::entity selectedEntity = entt::null;
	if (auto* hierarchyWindow = dynamic_cast<HierarchyWindow*>(context.GetEditor().GetWindow("Hierarchy")))
	{
		selectedEntity = hierarchyWindow->GetSelectedEntity();
	}

	auto view = registry.view<Component::LevelEntityInstance, Component::Transform>(entt::exclude<System::Inactive>);
	for (auto [entity, instance, transform] : view.each())
	{
		bool isSelected = entity == selectedEntity;
		bool isHovered  = entity == m_hoveredEntity;
		bool hasSprite  = registry.all_of<Component::Sprite>(entity);
		if (!isSelected && !isHovered && (hasSprite || !m_overlay.placeholderLabels))
		{
			continue;
		}
		glm::vec2 min    = glm::vec2(transformSystem.GetWorldPosition(context, entity)) + instance.offset;
		ImVec2 screenMin = toScreen(min);
		ImVec2 screenMax = toScreen(min + instance.size);
		if (isSelected || isHovered)
		{
			drawList->AddRect(screenMin, screenMax, isSelected ? SELECTED_COLOR : HOVERED_COLOR, 0.f, 0, 2.f);
		}
		if (!hasSprite && m_overlay.placeholderLabels)
		{
			DrawLabel(drawList, ImVec2(screenMin.x, screenMax.y + 2.f), IM_COL32_WHITE, GetDisplayName(instance));
		}
	}

	// Status line: the level and grid cell under the mouse, its world position, and zoom.
	std::string status = std::format("Zoom {:.0f}%", camera.zoom * 100.f);
	if (m_isMouseOverView)
	{
		status = std::format("{:.0f}, {:.0f} px   ", m_mouseWorldPos.x, m_mouseWorldPos.y) + status;
		if (m_hoveredLevel >= 0)
		{
			const auto& level = levels[m_hoveredLevel];
			int gridSize      = level.layers.empty() ? 0 : level.layers.front().gridSize;
			std::string cell;
			if (gridSize > 0)
			{
				cell = std::format("   cell {}, {}", (int)((m_mouseWorldPos.x - level.worldX) / gridSize),
				                   (int)((m_mouseWorldPos.y - level.worldY) / gridSize));
			}
			status = level.identifier + cell + "   " + status;
		}
	}
	float lineHeight = ImGui::GetTextLineHeight() + LABEL_PADDING * 2;
	DrawLabel(drawList, ImVec2(imagePos.x + 6.f, imagePos.y + imageSize.y - lineHeight - 6.f), IM_COL32_WHITE,
	          status);

	drawList->PopClipRect();
}

void LevelViewer::FrameRect(GameContext& context, float x, float y, float width, float height)
{
	Core::GameData& gameData = context.GetGameData();
	m_camera.FrameRect(glm::vec2(x, y), glm::vec2(width, height),
	                   glm::vec2((float)gameData.gameWidth, (float)gameData.gameHeight));
	m_camera.Apply(context);
}

void LevelViewer::FrameWorld(GameContext& context)
{
	auto* worldComponent = context.GetRegistry().try_get<Component::World>(m_worldEntity);
	if (!worldComponent || worldComponent->worldMap.levels.empty())
	{
		return;
	}

	glm::vec2 min{std::numeric_limits<float>::max()};
	glm::vec2 max{std::numeric_limits<float>::lowest()};
	for (const auto& level : worldComponent->worldMap.levels)
	{
		min = glm::min(min, glm::vec2(level.worldX, level.worldY));
		max = glm::max(max, glm::vec2(level.worldX + level.pxWid, level.worldY + level.pxHei));
	}
	FrameRect(context, min.x, min.y, max.x - min.x, max.y - min.y);
}

void LevelViewer::FrameLevel(GameContext& context, int levelIndex)
{
	auto* worldComponent = context.GetRegistry().try_get<Component::World>(m_worldEntity);
	if (!worldComponent || levelIndex < 0 || levelIndex >= (int)worldComponent->worldMap.levels.size())
	{
		return;
	}
	const auto& level = worldComponent->worldMap.levels[levelIndex];
	FrameRect(context, (float)level.worldX, (float)level.worldY, (float)level.pxWid, (float)level.pxHei);
}

void LevelViewer::SelectLevel(GameContext& context, int levelIndex)
{
	m_selectedLevel = levelIndex;

	entt::entity levelEntity = entt::null;
	if (auto* worldComponent = context.GetRegistry().try_get<Component::World>(m_worldEntity))
	{
		auto it = worldComponent->loadedLevels.find(levelIndex);
		if (it != worldComponent->loadedLevels.end())
		{
			levelEntity = it->second;
		}
	}
	SelectEntity(context, levelEntity);
}

void LevelViewer::SelectEntity(GameContext& context, entt::entity entity)
{
	if (auto* hierarchyWindow = dynamic_cast<HierarchyWindow*>(context.GetEditor().GetWindow("Hierarchy")))
	{
		hierarchyWindow->SetSelectedEntity(entity);
	}
}

bool LevelViewer::IsLevelVisible(const std::string& levelIid) const
{
	return !m_hiddenLevels.contains(levelIid);
}

void LevelViewer::SetLevelVisible(GameContext& context, const std::string& levelIid, bool visible)
{
	if (visible)
	{
		m_hiddenLevels.erase(levelIid);
	}
	else
	{
		m_hiddenLevels.insert(levelIid);
	}
	ApplyVisibility(context);
}

bool LevelViewer::IsLayerVisible(const std::string& layerIdentifier) const
{
	return !m_hiddenLayers.contains(layerIdentifier);
}

void LevelViewer::SetLayerVisible(GameContext& context, const std::string& layerIdentifier, bool visible)
{
	if (visible)
	{
		m_hiddenLayers.erase(layerIdentifier);
	}
	else
	{
		m_hiddenLayers.insert(layerIdentifier);
	}
	ApplyVisibility(context);
}

void LevelViewer::SetEntitiesVisible(GameContext& context, bool visible)
{
	m_entitiesVisible = visible;
	ApplyVisibility(context);
}

void LevelViewer::ApplyVisibility(GameContext& context)
{
	entt::registry& registry = context.GetRegistry();
	auto* worldComponent     = registry.try_get<Component::World>(m_worldEntity);
	if (!worldComponent)
	{
		return;
	}

	auto& hierarchySystem = context.GetSystemManager().GetSystem<System::HierarchySystem>();
	auto setActive        = [&](entt::entity entity, bool active)
	{
		if (active)
		{
			hierarchySystem.ActivevateEntity(context, entity);
		}
		else
		{
			hierarchySystem.DeactivevateEntity(context, entity);
		}
	};

	for (auto& [levelIndex, levelEntity] : worldComponent->loadedLevels)
	{
		// Children first: (de)activating the level afterwards only overrides children that aren't hidden themselves.
		if (auto* children = registry.try_get<Component::Children>(levelEntity))
		{
			for (entt::entity child : children->entities)
			{
				if (registry.all_of<Component::LevelEntityInstance>(child))
				{
					setActive(child, m_entitiesVisible);
				}
				else if (registry.all_of<Component::TileMap>(child))
				{
					setActive(child, IsLayerVisible(registry.get<Component::Identifier>(child).type));
				}
			}
		}
		setActive(levelEntity, IsLevelVisible(worldComponent->worldMap.levels[levelIndex].Iid));
	}
}

int LevelViewer::FindLevelAt(GameContext& context, const glm::vec2& worldPos) const
{
	auto* worldComponent = context.GetRegistry().try_get<Component::World>(m_worldEntity);
	if (!worldComponent)
	{
		return -1;
	}
	const auto& levels = worldComponent->worldMap.levels;
	for (int i = 0; i < (int)levels.size(); i++)
	{
		const auto& level = levels[i];
		if (IsLevelVisible(level.Iid) && worldPos.x >= level.worldX && worldPos.y >= level.worldY &&
		    worldPos.x < level.worldX + level.pxWid && worldPos.y < level.worldY + level.pxHei)
		{
			return i;
		}
	}
	return -1;
}
}  // namespace Struktur::Debug
