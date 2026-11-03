#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "entt/entt.hpp"
#include "glm/glm.hpp"

#include "Debug/Editor/LevelViewer/EditorCamera.h"

struct ImDrawList;

namespace Struktur
{
class GameContext;

namespace Debug
{
class GameViewportWindow;

// Shows an LDtk world in the Game Viewport while the game is stopped - every room's tiles and each entity's sprite
// (see World::Level::EntityMode::PREVIEW), nothing simulating (GameLoop only updates systems while
// debugSettings.playingGame is set; render systems always run).
// Lives in the normal registry, so pressing Play (StartDebugGame -> ClearGameSystems) wipes it like any other
// scene; the toolbar calls OnPlayStarted/OnPlayStopped so Stop brings the viewer back.
class LevelViewer
{
public:
	struct OverlaySettings
	{
		bool levelNames     = true;
		bool neighbourLinks = false;
		// Names of entities shown as placeholders (no sprite to recognise them by).
		bool placeholderLabels = true;
	};

	struct Options
	{
		// Re-open the file when it changes on disk (e.g. saved from LDtk), keeping the camera and visibility.
		bool autoReload = true;
		// Play the preview sprites' animations (only AnimationSystem runs - nothing else simulates).
		bool animatePreviews = false;
	};

	// Stops the game if it is running, clears the scene and loads every level of the world at filePath (a
	// virtual path, e.g. "Levels/SevenGods.ldtk"). Returns to where the camera last was for that file this
	// session, otherwise frames the whole world.
	void Open(GameContext& context, const std::string& filePath);
	// Clears the scene (back to the plain stopped editor).
	void Close(GameContext& context);
	// Re-opens the current file, e.g. after editing it in LDtk.
	void Reload(GameContext& context);

	// Play is about to wipe the scene - forget the world entity, but remember to come back on Stop.
	void OnPlayStarted();
	void OnPlayStopped(GameContext& context);

	// Called once per editor frame, after the windows have rendered: camera input, hover and click-to-select over
	// the Game Viewport (Ctrl+click: PlayFrom), auto-reload and preview animation.
	void Update(GameContext& context);
	// Level names, neighbour links, hover/selection outlines and the status line, drawn over the Game Viewport's
	// image (imagePos/imageSize in ImGui screen pixels). Called by GameViewportWindow::Render.
	void RenderViewportOverlay(GameContext& context, ImDrawList* drawList, const glm::vec2& imagePos,
	                           const glm::vec2& imageSize);

	bool IsActive() const
	{
		return m_worldEntity != entt::null;
	}
	const std::string& GetFilePath() const
	{
		return m_filePath;
	}
	entt::entity GetWorldEntity() const
	{
		return m_worldEntity;
	}

	// Points the camera at a world-space rect (px), fitting it to the game view.
	void FrameRect(GameContext& context, float x, float y, float width, float height);
	// FrameRect around every level of the open world.
	void FrameWorld(GameContext& context);
	void FrameLevel(GameContext& context, int levelIndex);

	// Selects the level (index into the world's levels, -1 for none) and its entity in the Hierarchy/Inspector.
	void SelectLevel(GameContext& context, int levelIndex);
	int GetSelectedLevel() const
	{
		return m_selectedLevel;
	}

	// Visibility, kept for the session across reloads and files. Levels are keyed by Iid, tile layers by identifier.
	bool IsLevelVisible(const std::string& levelIid) const;
	void SetLevelVisible(GameContext& context, const std::string& levelIid, bool visible);
	bool IsLayerVisible(const std::string& layerIdentifier) const;
	void SetLayerVisible(GameContext& context, const std::string& layerIdentifier, bool visible);
	bool AreEntitiesVisible() const
	{
		return m_entitiesVisible;
	}
	void SetEntitiesVisible(GameContext& context, bool visible);

	OverlaySettings& GetOverlaySettings()
	{
		return m_overlay;
	}
	Options& GetOptions()
	{
		return m_options;
	}

	// Starts the game (like the toolbar's Play) with the player at worldPos - see Core::GameData::debugSpawnWorldFile
	// for which game state honours it. Stop comes back to the viewer.
	void PlayFrom(GameContext& context, const glm::vec2& worldPos);

private:
	// (De)activates every level, tile layer and entity to match the visibility settings.
	void ApplyVisibility(GameContext& context);
	void UpdateHover(GameContext& context, const GameViewportWindow& viewport);
	void SelectEntity(GameContext& context, entt::entity entity);
	// Index of the visible level containing worldPos, or -1.
	int FindLevelAt(GameContext& context, const glm::vec2& worldPos) const;
	// Reloads once the file has changed on disk and stayed unchanged for a poll, and parses - LDtk may still be
	// writing it when the change is first seen. True if it reloaded.
	bool PollForFileChanges(GameContext& context);

	std::string m_filePath;
	entt::entity m_worldEntity = entt::null;
	bool m_reopenAfterPlay     = false;

	EditorCamera m_camera;
	// Last camera view per world file this session.
	std::unordered_map<std::string, EditorCamera::View> m_savedViews;

	std::unordered_set<std::string> m_hiddenLevels;
	std::unordered_set<std::string> m_hiddenLayers;
	bool m_entitiesVisible = true;
	OverlaySettings m_overlay;
	Options m_options;

	// Modification time of the open file as of the last (re)load, the changed time last seen by a poll, and time
	// since that poll (seconds).
	int64_t m_fileModTime    = 0;
	int64_t m_pendingModTime = 0;
	float m_timeSincePoll    = 0.f;

	int m_selectedLevel = -1;
	// Under the mouse as of the last Update - entity is a visible LevelEntityInstance, level an index or -1.
	entt::entity m_hoveredEntity = entt::null;
	int m_hoveredLevel           = -1;
	bool m_isMouseOverView       = false;
	glm::vec2 m_mouseWorldPos{0.f};
};
}  // namespace Debug
}  // namespace Struktur
