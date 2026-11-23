#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "entt/entt.hpp"
#include "glm/glm.hpp"

#include "Debug/Editor/LevelViewer/EditorCamera.h"
#include "Debug/Editor/LevelViewer/LdtkDocument.h"

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
		// Play the preview sprites' animations (only AnimationSystem runs - nothing else simulates). Particle effects
		// always play: an emitter shows nothing until its particles are simulated.
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

	// Actions that replace the scene. They run straight away - or, while the open file has unsaved edits, once the
	// unsaved-changes prompt is answered (Save / Discard; Cancel drops the action).
	void RequestOpen(GameContext& context, const std::string& filePath);
	void RequestReload(GameContext& context);
	void RequestClose(GameContext& context);
	// The toolbar's Play.
	void RequestPlay(GameContext& context);
	// Play with the player at worldPos - see Core::GameData::debugSpawnWorldFile for which game state honours it.
	// Stop comes back to the viewer.
	void RequestPlayFrom(GameContext& context, const glm::vec2& worldPos);

	// Editing the open file (see LdtkDocument). Save's outcome is the status message.
	bool HasUnsavedChanges() const
	{
		return m_document.HasUnsavedChanges();
	}
	bool CanUndo() const
	{
		return m_document.CanUndo();
	}
	bool CanRedo() const
	{
		return m_document.CanRedo();
	}
	bool Save(GameContext& context);
	void Undo(GameContext& context);
	void Redo(GameContext& context);
	// Field edits from the Inspector (EntityFieldEditor). The entity's preview is rebuilt from its new fields at the
	// next Update, after the windows have drawn. mergeWithLastEdit continues the previous undo step (a drag or typing
	// session still in progress).
	void SetEntityField(GameContext& context, const std::string& entityIid, const std::string& fieldIdentifier,
	                    const nlohmann::ordered_json& value, bool mergeWithLastEdit);
	void ResetEntityField(GameContext& context, const std::string& entityIid, const std::string& fieldIdentifier);
	// Clears a previewed particle effect's particles and fires its burst again (BurstCount particles at once).
	void ReplayParticles(GameContext& context, entt::entity entity);
	// Ctrl+S save, Ctrl+Z undo, Ctrl+Y / Ctrl+Shift+Z redo - called while the Game Viewport, the Level Viewer window
	// or the Inspector (showing an LDtk entity) has the user's attention.
	void HandleShortcuts(GameContext& context);
	LdtkDocument& GetDocument()
	{
		return m_document;
	}
	// The last save/load outcome or warning, for the Level Viewer window.
	const std::string& GetStatusMessage() const
	{
		return m_statusMessage;
	}
	bool IsStatusError() const
	{
		return m_isStatusError;
	}

private:
	enum class PendingAction
	{
		NONE,
		OPEN,
		RELOAD,
		CLOSE,
		PLAY,
		PLAY_FROM,
	};

	// Parts of the selected entity's move gizmo
	enum class GizmoHandle
	{
		NONE,
		MOVE_X,
		MOVE_Y,
		MOVE_FREE,
	};

	void Request(GameContext& context, PendingAction action, const std::string& filePath, const glm::vec2& worldPos);
	void RunPendingAction(GameContext& context);
	void RenderUnsavedChangesPrompt(GameContext& context);
	// Starts the game (debugSpawnWorldFile already set or cleared by the caller); Stop comes back to the viewer.
	void StartGame(GameContext& context);
	void SetStatus(const std::string& message, bool isError);
	// Brings the preview of an entity back in line with the document after an edit, undo or redo: its position, or -
	// queued for the next Update - its fields (args rebuilt, preview re-run).
	void RefreshEntity(GameContext& context, const std::string& entityIid);
	void RefreshAfterEdit(GameContext& context, const LdtkDocument::Edit& edit);
	void RefreshEntityFields(GameContext& context, const std::string& entityIid);
	// The preview entity of the LDtk entity with this iid, or entt::null.
	entt::entity FindPreviewEntity(GameContext& context, const std::string& entityIid) const;
	// Puts a preview entity where World::Level::LoadLevelEntities would for LDtk position `position`: the centre of
	// its rect (px - pivot * size + size / 2).
	void PlacePreview(GameContext& context, entt::entity entity, const std::string& entityIid,
	                  const glm::ivec2& position);

	// Moving entities. The selected entity shows a move gizmo: dragging its X or Y arrow moves the entity along that
	// axis, its centre square moves it freely, and one MoveEntity is recorded when the button is released (Escape
	// cancels). The arrow keys nudge the selected entity. Both snap the entity's top-left corner to its layer grid
	// unless Shift is held (then 1 px) and keep its rect inside its level.
	void StartEntityDrag(GameContext& context, entt::entity entity, GizmoHandle handle);
	void UpdateEntityDrag(GameContext& context);
	void EndEntityDrag();
	void NudgeSelectedEntity(GameContext& context);
	// Where an entity moved to `position` (LDtk px, level-relative) lands: its top-left corner snapped to its layer
	// grid if snapToGrid, and its rect inside its level.
	glm::ivec2 ConstrainEntityPosition(const std::string& entityIid, const glm::vec2& position, bool snapToGrid) const;

	// The Hierarchy's selected entity if it's an LDtk entity in the open document, else entt::null.
	entt::entity GetSelectedDocumentEntity(GameContext& context) const;
	// Where a world position is drawn on screen, in the Game Viewport image as of the last overlay draw.
	glm::vec2 WorldToViewScreen(GameContext& context, const glm::vec2& worldPos) const;
	// The selected entity's gizmo handle under the screen position, or NONE.
	GizmoHandle FindGizmoHandleAt(GameContext& context, const glm::vec2& screenPos) const;
	void DrawGizmo(GameContext& context, ImDrawList* drawList) const;

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
	LdtkDocument m_document;

	PendingAction m_pendingAction = PendingAction::NONE;
	std::string m_pendingFilePath;
	glm::vec2 m_pendingWorldPos{0.f};
	bool m_shouldOpenUnsavedPrompt = false;
	std::string m_statusMessage;
	bool m_isStatusError = false;
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

	// The Game Viewport image rect (screen px) as of the last overlay draw - for mapping the gizmo to the screen.
	glm::vec2 m_viewImagePos{0.f};
	glm::vec2 m_viewImageSize{0.f};
	// The gizmo handle under the mouse as of the last Update (NONE while dragging), and the one being dragged.
	GizmoHandle m_hoveredGizmoHandle = GizmoHandle::NONE;
	GizmoHandle m_dragHandle         = GizmoHandle::NONE;

	// The entity being dragged by its gizmo (entt::null when none), where it started, where it is now and where the
	// mouse was when the button went down. It only moves once the mouse has travelled past ImGui's drag threshold.
	entt::entity m_dragEntity = entt::null;
	std::string m_dragEntityIid;
	glm::ivec2 m_dragStartPosition{0};
	glm::ivec2 m_dragPosition{0};
	glm::vec2 m_dragStartMouse{0.f};
	bool m_isDragMoving = false;

	// Entities whose fields changed since the last Update - see RefreshEntityFields.
	std::unordered_set<std::string> m_entitiesToRefresh;
};
}  // namespace Debug
}  // namespace Struktur
