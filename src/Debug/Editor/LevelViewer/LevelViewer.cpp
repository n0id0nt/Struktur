#include "LevelViewer.h"

#include <imgui.h>

#include <algorithm>
#include <format>
#include <limits>

#include "Debug/Assertions.h"
#include "Debug/Editor/Editor.h"
#include "Debug/Editor/Windows/GameViewportWindow.h"
#include "Debug/Editor/Windows/HierarchyWindow.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/LevelEntityInstance.h"
#include "Engine/ECS/Component/ParticleEmitter.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/World.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/HierarchySystem.h"
#include "Engine/ECS/System/ParticleSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/Game.h"
#include "Engine/GameContext.h"
#include "Engine/World/Level.h"
#include "nlohmann/json.hpp"

namespace Struktur::Debug
{
static constexpr ImU32 LEVEL_NAME_COLOR     = IM_COL32(255, 203, 0, 255);
static constexpr ImU32 LABEL_BACKGROUND     = IM_COL32(0, 0, 0, 170);
static constexpr ImU32 SELECTED_COLOR       = IM_COL32(102, 191, 255, 255);
static constexpr ImU32 HOVERED_COLOR        = IM_COL32(255, 255, 255, 200);
static constexpr ImU32 NEIGHBOUR_LINK_COLOR = IM_COL32(255, 161, 0, 160);
static constexpr float LABEL_PADDING        = 3.f;

// Move gizmo, in screen px - the same size at any zoom.
static constexpr ImU32 GIZMO_X_COLOR          = IM_COL32(235, 75, 75, 255);
static constexpr ImU32 GIZMO_Y_COLOR          = IM_COL32(95, 210, 95, 255);
static constexpr ImU32 GIZMO_FREE_COLOR       = IM_COL32(102, 191, 255, 200);
static constexpr ImU32 GIZMO_ACTIVE_COLOR     = IM_COL32(255, 220, 60, 255);
static constexpr float GIZMO_AXIS_LENGTH      = 64.f;
static constexpr float GIZMO_ARROW_HEAD       = 11.f;
static constexpr float GIZMO_LINE_THICKNESS   = 3.f;
static constexpr float GIZMO_PICK_TOLERANCE   = 6.f;
static constexpr float GIZMO_CENTRE_HALF_SIZE = 7.f;
// How often the open file's modification time is checked (seconds).
static constexpr float FILE_POLL_INTERVAL = 0.5f;
static constexpr const char* UNSAVED_PROMPT_ID = "Unsaved level edits";

// The parsed copy (World component's worldMap) of the LDtk entity with this iid, or null.
static FileLoading::LevelParser::Entity* FindParsedEntity(FileLoading::LevelParser::World& world,
                                                          const std::string& entityIid)
{
	for (auto& level : world.levels)
	{
		for (auto& layer : level.layers)
		{
			for (auto& entity : layer.entityInstaces)
			{
				if (entity.Iid == entityIid)
				{
					return &entity;
				}
			}
		}
	}
	return nullptr;
}

// A level's centre in world pixels.
static float DistanceToSegment(const glm::vec2& point, const glm::vec2& segmentStart, const glm::vec2& segmentEnd)
{
	glm::vec2 segment   = segmentEnd - segmentStart;
	float lengthSquared = glm::dot(segment, segment);
	float t = lengthSquared > 0.f ? glm::clamp(glm::dot(point - segmentStart, segment) / lengthSquared, 0.f, 1.f) : 0.f;
	glm::vec2 closestPoint = segmentStart + segment * t;
	return glm::length(point - closestPoint);
}

static ImVec2 ToImVec2(const glm::vec2& vector)
{
	return ImVec2(vector.x, vector.y);
}

// A gizmo axis: a line from origin with a triangular head pointing along direction (unit, screen space).
static void DrawGizmoArrow(ImDrawList* drawList, const glm::vec2& origin, const glm::vec2& direction, ImU32 color)
{
	glm::vec2 tip      = origin + direction * GIZMO_AXIS_LENGTH;
	glm::vec2 headBase = tip - direction * GIZMO_ARROW_HEAD;
	glm::vec2 perpendicular(-direction.y, direction.x);
	drawList->AddLine(ToImVec2(origin + direction * GIZMO_CENTRE_HALF_SIZE), ToImVec2(headBase), color,
	                  GIZMO_LINE_THICKNESS);
	drawList->AddTriangleFilled(ToImVec2(tip), ToImVec2(headBase + perpendicular * (GIZMO_ARROW_HEAD * 0.5f)),
	                            ToImVec2(headBase - perpendicular * (GIZMO_ARROW_HEAD * 0.5f)), color);
}

static glm::vec2 LevelCentre(const FileLoading::LevelParser::Level& level)
{
	return glm::vec2(level.worldX + level.pxWid / 2.f, level.worldY + level.pxHei / 2.f);
}

// The "Name" field if the instance has one (most do - NPC, Item, ...), otherwise its LDtk identifier.
static std::string GetDisplayName(const Component::LevelEntityInstance& instance)
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

static void DrawLabel(ImDrawList* drawList, const ImVec2& position, ImU32 color, const std::string& text)
{
	ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
	ImVec2 max(position.x + textSize.x + LABEL_PADDING * 2, position.y + textSize.y + LABEL_PADDING * 2);
	drawList->AddRectFilled(position, max, LABEL_BACKGROUND);
	drawList->AddText(ImVec2(position.x + LABEL_PADDING, position.y + LABEL_PADDING), color, text.c_str());
}

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
	EndEntityDrag();

	m_filePath    = filePath;
	m_worldEntity = World::Level::CreateWorldEntity(context, filePath);

	auto modTime     = FileSystem::GetModificationTime(filePath);
	m_fileModTime    = modTime.success ? modTime.value : 0;
	m_pendingModTime = 0;
	m_timeSincePoll  = 0.f;

	std::string loadError;
	if (m_document.Load(filePath, loadError))
	{
		SetStatus("", false);
	}
	else
	{
		DEBUG_ERROR("[Level Viewer] %s", loadError.c_str());
		SetStatus(loadError + " - editing is off", true);
	}

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
	m_document.Clear();
	EndEntityDrag();
	SetStatus("", false);
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
	EndEntityDrag();
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
	RenderUnsavedChangesPrompt(context);
	// The prompt's answer may have replaced or wiped the scene.
	if (!IsActive() || context.GetEditor().GetSettings().debugRender.playingGame)
	{
		return;
	}
	for (const std::string& entityIid : m_entitiesToRefresh)
	{
		RefreshEntityFields(context, entityIid);
	}
	m_entitiesToRefresh.clear();

	GameViewportWindow* viewport = context.GetEditor().GetGameViewport();
	if (viewport)
	{
		m_camera.HandleInput(context, *viewport);

		ImGuiIO& io = ImGui::GetIO();
		if ((viewport->IsViewportHovered() || viewport->IsViewportFocused()) && !io.WantTextInput)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_F, false) && !io.KeyCtrl)
			{
				FrameWorld(context);
			}
			if (m_dragEntity == entt::null)
			{
				HandleShortcuts(context);
				NudgeSelectedEntity(context);
			}
		}
	}

	m_camera.Apply(context);
	m_savedViews[m_filePath] = m_camera.GetView();

	context.GetSystemManager().GetSystem<System::ParticleSystem>().Update(context);
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

	// The gizmo sits on top of everything - over a handle, the entity underneath isn't hovered or picked.
	m_hoveredGizmoHandle = GizmoHandle::NONE;
	if (m_isMouseOverView && m_dragEntity == entt::null)
	{
		ImVec2 mousePos      = ImGui::GetMousePos();
		m_hoveredGizmoHandle = FindGizmoHandleAt(context, glm::vec2(mousePos.x, mousePos.y));
		if (m_hoveredGizmoHandle != GizmoHandle::NONE)
		{
			m_hoveredEntity = entt::null;
		}
	}

	// The size-preset buttons float over the image - a click on one of those isn't a pick.
	if (m_isMouseOverView && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
	{
		if (ImGui::GetIO().KeyCtrl)
		{
			RequestPlayFrom(context, m_mouseWorldPos);
			return;
		}
		if (m_hoveredGizmoHandle != GizmoHandle::NONE)
		{
			StartEntityDrag(context, GetSelectedDocumentEntity(context), m_hoveredGizmoHandle);
		}
		else if (m_hoveredEntity != entt::null)
		{
			m_selectedLevel = m_hoveredLevel;
			SelectEntity(context, m_hoveredEntity);
		}
		else
		{
			SelectLevel(context, m_hoveredLevel);
		}
	}
	UpdateEntityDrag(context);

	if (m_hoveredEntity != entt::null && !m_isDragMoving)
	{
		const auto& instance = context.GetRegistry().get<Component::LevelEntityInstance>(m_hoveredEntity);
		ImGui::SetTooltip("%s (%s)", GetDisplayName(instance).c_str(), instance.identifier.c_str());
	}
}

void LevelViewer::RequestOpen(GameContext& context, const std::string& filePath)
{
	Request(context, PendingAction::OPEN, filePath, glm::vec2(0.f));
}

void LevelViewer::RequestReload(GameContext& context)
{
	Request(context, PendingAction::RELOAD, "", glm::vec2(0.f));
}

void LevelViewer::RequestClose(GameContext& context)
{
	Request(context, PendingAction::CLOSE, "", glm::vec2(0.f));
}

void LevelViewer::RequestPlay(GameContext& context)
{
	Request(context, PendingAction::PLAY, "", glm::vec2(0.f));
}

void LevelViewer::RequestPlayFrom(GameContext& context, const glm::vec2& worldPos)
{
	Request(context, PendingAction::PLAY_FROM, "", worldPos);
}

void LevelViewer::Request(GameContext& context, PendingAction action, const std::string& filePath,
                          const glm::vec2& worldPos)
{
	m_pendingAction   = action;
	m_pendingFilePath = filePath;
	m_pendingWorldPos = worldPos;
	if (IsActive() && m_document.HasUnsavedChanges())
	{
		m_shouldOpenUnsavedPrompt = true;
		return;
	}
	RunPendingAction(context);
}

void LevelViewer::RunPendingAction(GameContext& context)
{
	PendingAction action = m_pendingAction;
	m_pendingAction      = PendingAction::NONE;
	switch (action)
	{
		case PendingAction::OPEN:
			Open(context, m_pendingFilePath);
			break;
		case PendingAction::RELOAD:
			Reload(context);
			break;
		case PendingAction::CLOSE:
			Close(context);
			break;
		case PendingAction::PLAY:
			context.GetGameData().debugSpawnWorldFile.clear();
			StartGame(context);
			break;
		case PendingAction::PLAY_FROM:
		{
			Core::GameData& gameData     = context.GetGameData();
			gameData.debugSpawnWorldFile = m_filePath;
			gameData.debugSpawnPosition  = m_pendingWorldPos;
			DEBUG_INFO("[Level Viewer] Play from %s (%.0f, %.0f)", m_filePath.c_str(), m_pendingWorldPos.x,
			           m_pendingWorldPos.y);
			StartGame(context);
			break;
		}
		case PendingAction::NONE:
			break;
	}
}

void LevelViewer::RenderUnsavedChangesPrompt(GameContext& context)
{
	if (m_shouldOpenUnsavedPrompt)
	{
		ImGui::OpenPopup(UNSAVED_PROMPT_ID);
		m_shouldOpenUnsavedPrompt = false;
	}
	if (!ImGui::BeginPopupModal(UNSAVED_PROMPT_ID, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		return;
	}

	// Run after EndPopup - the action may clear the scene this prompt is about.
	bool shouldRunAction = false;
	ImGui::Text("%s has unsaved edits.", m_filePath.c_str());
	if (m_isStatusError && !m_statusMessage.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s", m_statusMessage.c_str());
	}
	if (ImGui::Button("Save"))
	{
		if (Save(context))
		{
			shouldRunAction = true;
			ImGui::CloseCurrentPopup();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Discard"))
	{
		shouldRunAction = true;
		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		m_pendingAction = PendingAction::NONE;
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();

	if (shouldRunAction)
	{
		// Every action reloads the document from disk (or drops it), so discarding needs nothing more.
		RunPendingAction(context);
	}
}

void LevelViewer::StartGame(GameContext& context)
{
	auto& debugSettings       = context.GetEditor().GetSettings().debugRender;
	debugSettings.playingGame = true;
	debugSettings.pausedGame  = false;
	OnPlayStarted();
	StartDebugGame(context);
}

bool LevelViewer::Save(GameContext& context)
{
	std::string saveError;
	if (!m_document.Save(saveError))
	{
		DEBUG_ERROR("[Level Viewer] Save failed: %s", saveError.c_str());
		SetStatus("Save failed: " + saveError, true);
		return false;
	}
	// Our own write - not an outside change for auto-reload to pick up.
	auto modTime     = FileSystem::GetModificationTime(m_filePath);
	m_fileModTime    = modTime.success ? modTime.value : m_fileModTime;
	m_pendingModTime = 0;
	DEBUG_INFO("[Level Viewer] Saved %s", m_filePath.c_str());
	SetStatus("Saved " + m_filePath, false);
	return true;
}

void LevelViewer::Undo(GameContext& context)
{
	if (const LdtkDocument::Edit* edit = m_document.Undo())
	{
		RefreshAfterEdit(context, *edit);
	}
}

void LevelViewer::Redo(GameContext& context)
{
	if (const LdtkDocument::Edit* edit = m_document.Redo())
	{
		RefreshAfterEdit(context, *edit);
	}
}

void LevelViewer::SetEntityField(GameContext& context, const std::string& entityIid,
                                 const std::string& fieldIdentifier, const nlohmann::ordered_json& value,
                                 bool mergeWithLastEdit)
{
	if (m_document.SetFieldValue(entityIid, fieldIdentifier, value, mergeWithLastEdit))
	{
		m_entitiesToRefresh.insert(entityIid);
	}
}

void LevelViewer::ResetEntityField(GameContext& context, const std::string& entityIid,
                                   const std::string& fieldIdentifier)
{
	if (m_document.ResetFieldToDefault(entityIid, fieldIdentifier))
	{
		m_entitiesToRefresh.insert(entityIid);
	}
}

void LevelViewer::ReplayParticles(GameContext& context, entt::entity entity)
{
	auto* emitter = context.GetRegistry().try_get<Component::ParticleEmitter>(entity);
	if (!emitter)
	{
		return;
	}
	for (Component::Particle& particle : emitter->particles)
	{
		particle.alive = false;
	}
	emitter->hasBurst         = false;
	emitter->spawnAccumulator = 0.0f;
}

void LevelViewer::RefreshAfterEdit(GameContext& context, const LdtkDocument::Edit& edit)
{
	if (edit.fieldIdentifier.empty())
	{
		RefreshEntity(context, edit.entityIid);
	}
	else
	{
		m_entitiesToRefresh.insert(edit.entityIid);
	}
}

void LevelViewer::RefreshEntityFields(GameContext& context, const std::string& entityIid)
{
	entt::registry& registry             = context.GetRegistry();
	entt::entity entity                  = FindPreviewEntity(context, entityIid);
	auto* worldComponent                 = registry.try_get<Component::World>(m_worldEntity);
	const nlohmann::ordered_json* edited = m_document.FindEntity(entityIid);
	if (entity == entt::null || !worldComponent || !edited)
	{
		return;
	}
	FileLoading::LevelParser::Entity* parsedEntity = FindParsedEntity(worldComponent->worldMap, entityIid);
	if (!parsedEntity)
	{
		return;
	}

	// Same parsing and arg building as loading the level, so the preview sees exactly what it would after a reload.
	parsedEntity->fieldInstances.clear();
	FileLoading::LevelParser::LoadFieldInstances(parsedEntity->fieldInstances,
	                                             nlohmann::json::parse((*edited)["fieldInstances"].dump()));
	auto& instance  = registry.get<Component::LevelEntityInstance>(entity);
	instance.fields = World::Level::BuildEntityScriptArgs(*parsedEntity, worldComponent->worldMap);

	// Re-run the preview from scratch: drop what the last one made, then call it again.
	registry.remove<Component::Sprite, Component::SpriteAnimation, Component::ParticleEmitter>(entity);
	if (auto* children = registry.try_get<Component::Children>(entity))
	{
		std::vector<entt::entity> previewChildren = children->entities;
		auto& hierarchySystem                     = context.GetSystemManager().GetSystem<System::HierarchySystem>();
		for (entt::entity child : previewChildren)
		{
			hierarchySystem.DestroyEntity(context, child);
		}
	}
	context.GetSystemManager().GetSystem<System::WrenScriptSystem>().CallPreview(context, entity, instance.identifier,
	                                                                             instance.fields);
}

void LevelViewer::HandleShortcuts(GameContext& context)
{
	ImGuiIO& io = ImGui::GetIO();
	if (!io.KeyCtrl || io.WantTextInput)
	{
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_S, false))
	{
		Save(context);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Y, false) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z, false)))
	{
		Redo(context);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
	{
		Undo(context);
	}
}

void LevelViewer::SetStatus(const std::string& message, bool isError)
{
	m_statusMessage = message;
	m_isStatusError = isError;
}

void LevelViewer::RefreshEntity(GameContext& context, const std::string& entityIid)
{
	entt::entity entity = FindPreviewEntity(context, entityIid);
	if (entity == entt::null)
	{
		return;
	}
	PlacePreview(context, entity, entityIid, m_document.GetEntityPosition(entityIid));
}

void LevelViewer::PlacePreview(GameContext& context, entt::entity entity, const std::string& entityIid,
                               const glm::ivec2& position)
{
	// Matches World::Level::LoadLevelEntities: px is the pivot point, the entity sits at the centre of its rect.
	glm::vec2 size       = glm::vec2(m_document.GetEntitySize(entityIid));
	glm::vec2 rectCentre = glm::vec2(position) - m_document.GetEntityPivot(entityIid) * size + size * 0.5f;
	context.GetSystemManager().GetSystem<System::TransformSystem>().SetLocalTransform(
	    context, entity, glm::vec3(rectCentre, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
}

void LevelViewer::StartEntityDrag(GameContext& context, entt::entity entity, GizmoHandle handle)
{
	if (entity == entt::null || handle == GizmoHandle::NONE)
	{
		return;
	}
	const auto* instance = context.GetRegistry().try_get<Component::LevelEntityInstance>(entity);
	if (!instance || !m_document.HasEntity(instance->Iid))
	{
		return;
	}
	m_dragHandle        = handle;
	m_dragEntity        = entity;
	m_dragEntityIid     = instance->Iid;
	m_dragStartPosition = m_document.GetEntityPosition(instance->Iid);
	m_dragPosition      = m_dragStartPosition;
	m_dragStartMouse    = m_mouseWorldPos;
	m_isDragMoving      = false;
}

void LevelViewer::UpdateEntityDrag(GameContext& context)
{
	if (m_dragEntity == entt::null)
	{
		return;
	}
	if (!context.GetRegistry().valid(m_dragEntity))
	{
		EndEntityDrag();
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
	{
		PlacePreview(context, m_dragEntity, m_dragEntityIid, m_dragStartPosition);
		EndEntityDrag();
		return;
	}

	if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		if (!m_isDragMoving && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			return;
		}
		m_isDragMoving = true;
		// The mouse position stops updating while the cursor is off the viewport - the entity waits at the edge.
		glm::vec2 mouseDelta = m_mouseWorldPos - m_dragStartMouse;
		if (m_dragHandle == GizmoHandle::MOVE_X)
		{
			mouseDelta.y = 0.f;
		}
		else if (m_dragHandle == GizmoHandle::MOVE_Y)
		{
			mouseDelta.x = 0.f;
		}
		glm::vec2 target = glm::vec2(m_dragStartPosition) + mouseDelta;
		m_dragPosition   = ConstrainEntityPosition(m_dragEntityIid, target, !ImGui::GetIO().KeyShift);
		// An axis drag leaves the other axis exactly where it was, even if that's off the grid.
		if (m_dragHandle == GizmoHandle::MOVE_X)
		{
			m_dragPosition.y = m_dragStartPosition.y;
		}
		else if (m_dragHandle == GizmoHandle::MOVE_Y)
		{
			m_dragPosition.x = m_dragStartPosition.x;
		}
		PlacePreview(context, m_dragEntity, m_dragEntityIid, m_dragPosition);
		return;
	}

	// Released: one undoable move for the whole drag.
	if (m_isDragMoving && m_dragPosition != m_dragStartPosition)
	{
		m_document.MoveEntity(m_dragEntityIid, m_dragPosition);
	}
	RefreshEntity(context, m_dragEntityIid);
	EndEntityDrag();
}

void LevelViewer::EndEntityDrag()
{
	m_dragEntity = entt::null;
	m_dragEntityIid.clear();
	m_isDragMoving = false;
	m_dragHandle   = GizmoHandle::NONE;
}

void LevelViewer::NudgeSelectedEntity(GameContext& context)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.KeyCtrl || io.WantTextInput)
	{
		return;
	}
	auto* hierarchyWindow = dynamic_cast<HierarchyWindow*>(context.GetEditor().GetWindow("Hierarchy"));
	if (!hierarchyWindow)
	{
		return;
	}
	entt::entity selected = hierarchyWindow->GetSelectedEntity();
	if (selected == entt::null || !context.GetRegistry().valid(selected))
	{
		return;
	}
	const auto* instance = context.GetRegistry().try_get<Component::LevelEntityInstance>(selected);
	if (!instance || !m_document.HasEntity(instance->Iid))
	{
		return;
	}

	glm::ivec2 direction(0);
	if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
	{
		direction.x = -1;
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
	{
		direction.x = 1;
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		direction.y = -1;
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		direction.y = 1;
	}
	if (direction == glm::ivec2(0))
	{
		return;
	}

	int stepSize      = io.KeyShift ? 1 : m_document.GetEntityGridSize(instance->Iid);
	glm::ivec2 target = m_document.GetEntityPosition(instance->Iid) + direction * stepSize;
	// Not snapped: a grid-sized step keeps an on-grid entity on the grid, and leaves a 1 px-placed one where it was.
	target = ConstrainEntityPosition(instance->Iid, glm::vec2(target), false);
	if (m_document.MoveEntity(instance->Iid, target))
	{
		RefreshEntity(context, instance->Iid);
	}
}

glm::ivec2 LevelViewer::ConstrainEntityPosition(const std::string& entityIid, const glm::vec2& position,
                                                bool snapToGrid) const
{
	// Works on the rect's top-left corner (px - pivot * size) - that's what LDtk lines up with the grid.
	float gridSize        = (float)std::max(1, m_document.GetEntityGridSize(entityIid));
	glm::vec2 levelSize   = glm::vec2(m_document.GetEntityLevelSize(entityIid));
	glm::vec2 entitySize  = glm::vec2(m_document.GetEntitySize(entityIid));
	glm::vec2 pivotOffset = m_document.GetEntityPivot(entityIid) * entitySize;

	glm::vec2 topLeft = position - pivotOffset;
	topLeft           = snapToGrid ? glm::round(topLeft / gridSize) * gridSize : glm::round(topLeft);
	// The furthest the rect can go and still be inside the level - on the grid when snapping.
	glm::vec2 maximum = glm::max(levelSize - entitySize, glm::vec2(0.f));
	if (snapToGrid)
	{
		maximum = glm::floor(maximum / gridSize) * gridSize;
	}
	topLeft = glm::clamp(topLeft, glm::vec2(0.f), maximum);
	return glm::ivec2(glm::round(topLeft + pivotOffset));
}

entt::entity LevelViewer::GetSelectedDocumentEntity(GameContext& context) const
{
	auto* hierarchyWindow = dynamic_cast<HierarchyWindow*>(context.GetEditor().GetWindow("Hierarchy"));
	if (!hierarchyWindow)
	{
		return entt::null;
	}
	entt::entity selected = hierarchyWindow->GetSelectedEntity();
	entt::registry& registry = context.GetRegistry();
	if (selected == entt::null || !registry.valid(selected) || registry.all_of<System::Inactive>(selected))
	{
		return entt::null;
	}
	const auto* instance = registry.try_get<Component::LevelEntityInstance>(selected);
	if (!instance || !m_document.HasEntity(instance->Iid))
	{
		return entt::null;
	}
	return selected;
}

glm::vec2 LevelViewer::WorldToViewScreen(GameContext& context, const glm::vec2& worldPos) const
{
	Core::GameData& gameData = context.GetGameData();
	glm::vec2 gameToScreen   = m_viewImageSize / glm::vec2((float)gameData.gameWidth, (float)gameData.gameHeight);
	return m_viewImagePos + context.GetCamera().WorldPosToScreenPos(worldPos) * gameToScreen;
}

LevelViewer::GizmoHandle LevelViewer::FindGizmoHandleAt(GameContext& context, const glm::vec2& screenPos) const
{
	entt::entity selected = GetSelectedDocumentEntity(context);
	if (selected == entt::null || m_viewImageSize.x <= 0.f)
	{
		return GizmoHandle::NONE;
	}
	System::TransformSystem& transformSystem = context.GetSystemManager().GetSystem<System::TransformSystem>();
	glm::vec2 origin = WorldToViewScreen(context, glm::vec2(transformSystem.GetWorldPosition(context, selected)));

	glm::vec2 fromOrigin = glm::abs(screenPos - origin);
	if (fromOrigin.x <= GIZMO_CENTRE_HALF_SIZE + 1.f && fromOrigin.y <= GIZMO_CENTRE_HALF_SIZE + 1.f)
	{
		return GizmoHandle::MOVE_FREE;
	}
	if (DistanceToSegment(screenPos, origin, origin + glm::vec2(GIZMO_AXIS_LENGTH, 0.f)) <= GIZMO_PICK_TOLERANCE)
	{
		return GizmoHandle::MOVE_X;
	}
	if (DistanceToSegment(screenPos, origin, origin + glm::vec2(0.f, GIZMO_AXIS_LENGTH)) <= GIZMO_PICK_TOLERANCE)
	{
		return GizmoHandle::MOVE_Y;
	}
	return GizmoHandle::NONE;
}

void LevelViewer::DrawGizmo(GameContext& context, ImDrawList* drawList) const
{
	entt::entity selected = GetSelectedDocumentEntity(context);
	if (selected == entt::null)
	{
		return;
	}
	System::TransformSystem& transformSystem = context.GetSystemManager().GetSystem<System::TransformSystem>();
	glm::vec2 origin = WorldToViewScreen(context, glm::vec2(transformSystem.GetWorldPosition(context, selected)));

	// Highlight the handle being dragged, or else the one under the mouse.
	GizmoHandle highlighted = m_dragHandle != GizmoHandle::NONE ? m_dragHandle : m_hoveredGizmoHandle;
	ImU32 xColor            = highlighted == GizmoHandle::MOVE_X ? GIZMO_ACTIVE_COLOR : GIZMO_X_COLOR;
	ImU32 yColor            = highlighted == GizmoHandle::MOVE_Y ? GIZMO_ACTIVE_COLOR : GIZMO_Y_COLOR;
	ImU32 freeColor         = highlighted == GizmoHandle::MOVE_FREE ? GIZMO_ACTIVE_COLOR : GIZMO_FREE_COLOR;

	// World y runs down the screen (as in LDtk), so the Y arrow points down.
	DrawGizmoArrow(drawList, origin, glm::vec2(1.f, 0.f), xColor);
	DrawGizmoArrow(drawList, origin, glm::vec2(0.f, 1.f), yColor);
	glm::vec2 centreExtent(GIZMO_CENTRE_HALF_SIZE);
	drawList->AddRectFilled(ToImVec2(origin - centreExtent), ToImVec2(origin + centreExtent), freeColor);
	drawList->AddRect(ToImVec2(origin - centreExtent), ToImVec2(origin + centreExtent), IM_COL32(0, 0, 0, 200));
}

entt::entity LevelViewer::FindPreviewEntity(GameContext& context, const std::string& entityIid) const
{
	auto view = context.GetRegistry().view<Component::LevelEntityInstance>();
	for (auto [entity, instance] : view.each())
	{
		if (instance.Iid == entityIid)
		{
			return entity;
		}
	}
	return entt::null;
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
	// Reloading would throw the edits away - leave it to the user (Save refuses too: its baseline is stale).
	if (m_document.HasUnsavedChanges())
	{
		SetStatus(m_filePath + " changed on disk - reload to see it (your unsaved edits will be lost)", true);
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

	m_viewImagePos  = imagePos;
	m_viewImageSize = imageSize;

	drawList->PushClipRect(ImVec2(imagePos.x, imagePos.y), ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y),
	                       true);

	const auto& levels = worldComponent->worldMap.levels;
	if (m_overlay.neighbourLinks)
	{
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
				drawList->AddLine(toScreen(LevelCentre(levels[i])), toScreen(LevelCentre(levels[j])),
				                  NEIGHBOUR_LINK_COLOR, 2.f);
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

	DrawGizmo(context, drawList);

	// Status line: the level and grid cell under the mouse, its world position, and zoom.
	std::string status = std::format("Zoom {:.0f}%", camera.zoom * 100.f);
	if (m_isDragMoving)
	{
		status = std::format("Moving to {}, {} px (Shift: free, Esc: cancel)   ", m_dragPosition.x, m_dragPosition.y) +
		         status;
	}
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
