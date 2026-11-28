// The inspector panels of the 2D world's components: part of the editor in games with the world2d library.
#include <cmath>
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Debug/Editor/Editor.h"
#include "Debug/Editor/LevelViewer/EntityFieldEditor.h"
#include "Debug/Editor/PreviewRenderers/PreviewHelpers.h"
#include "Debug/Editor/Windows/PreviewWindow.h"
#include "Engine/Animation/SpriteAnimation.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/LevelEntityInstance.h"
#include "Engine/ECS/Component/ParticleEmitter.h"
#include "Engine/ECS/Component/Shader.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/World.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/FileLoading/LevelParser.h"
#include "Engine/GameContext.h"
#include "Engine/World/RenderLayer.h"
#include "InspectorWindow.h"

namespace Struktur::Debug
{
// A level entity field's value as text. Lists (LDtk Colour / Point fields) are shown as their named parts, e.g.
// "x: 3, y: 4".
static std::string FormatFieldValue(const Wren::WrenItem& item)
{
	switch (item.type)
	{
		case WrenType::WREN_TYPE_NUM:
			return std::format("{:.4g}", std::any_cast<double>(item.value));
		case WrenType::WREN_TYPE_BOOL:
			return std::any_cast<bool>(item.value) ? "true" : "false";
		case WrenType::WREN_TYPE_STRING:
			return "\"" + std::any_cast<std::string>(item.value) + "\"";
		case WrenType::WREN_TYPE_LIST:
		{
			std::string text;
			for (const auto& part : std::any_cast<std::vector<Wren::WrenItem>>(item.value))
			{
				text += (text.empty() ? "" : ", ") + part.identifier + ": " + FormatFieldValue(part);
			}
			return text;
		}
		default:
			return "null";
	}
}

void InspectorWindow::Render2DComponents(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();

	if (auto* sprite = registry.try_get<Component::Sprite>(entity))
	{
		if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Sprite");
			RenderSpriteComponent(context, *sprite, registry, entity);
			ImGui::PopID();
		}
	}

	if (auto* shader = registry.try_get<Component::Shader>(entity))
	{
		if (ImGui::CollapsingHeader("Shader", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Shader");
			RenderShaderComponent(context, *shader, registry, entity);
			ImGui::PopID();
		}
	}

	if (auto* camera = registry.try_get<Component::Camera>(entity))
	{
		if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Camera");
			RenderCameraComponent(context, *camera, registry, entity);
			ImGui::PopID();
		}
	}

	if (auto* particleEmitter = registry.try_get<Component::ParticleEmitter>(entity))
	{
		if (ImGui::CollapsingHeader("Particle Emitter", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("ParticleEmitter");
			RenderParticleEmitterComponent(context, *particleEmitter, registry, entity);
			ImGui::PopID();
		}
	}

	if (auto* spriteAnimation = registry.try_get<Component::SpriteAnimation>(entity))
	{
		if (ImGui::CollapsingHeader("Sprite Animation", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("SpriteAnimation");
			RenderSpriteAnimationComponent(context, *spriteAnimation, registry, entity);
			ImGui::PopID();
		}
	}

	if (auto* level = registry.try_get<Component::Level>(entity))
	{
		if (ImGui::CollapsingHeader("Level"))
		{
			ImGui::PushID("Level");
			RenderLevelComponent(context, *level, registry, entity);
			ImGui::PopID();
		}
	}

	if (auto* instance = registry.try_get<Component::LevelEntityInstance>(entity))
	{
		if (ImGui::CollapsingHeader("LDtk Entity", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("LDtk Entity");
			RenderLevelEntityInstanceComponent(context, *instance, registry, entity);
			ImGui::PopID();
		}
	}

	if (auto* world = registry.try_get<Component::World>(entity))
	{
		if (ImGui::CollapsingHeader("World"))
		{
			ImGui::PushID("World");
			RenderWorldComponent(context, *world, registry, entity);
			ImGui::PopID();
		}
	}

	if (auto* tileMap = registry.try_get<Component::TileMap>(entity))
	{
		if (ImGui::CollapsingHeader("Tile Map", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("TileMap");
			RenderTileMapComponent(context, *tileMap, registry, entity);
			ImGui::PopID();
		}
	}
}

void InspectorWindow::RenderCameraComponent(GameContext& context, Component::Camera& camera, entt::registry& registry,
                                            entt::entity entity)
{
	ImGui::DragInt("Priority", &camera.cameraPriority);
	ImGui::DragFloat("Zoom", &camera.zoom, 0.01f, 0.01f, 100.0f);
	ImGui::DragFloat("Angle", &camera.angle, 0.5f);

	ImGui::Spacing();

	RenderVec2("Offset", camera.offset);
	RenderVec2("Dead Zone", camera.deadZone);
	RenderVec2("Damping", camera.damping);
	ImGui::Checkbox("Force Position", &camera.forcePosition);
	ImGui::Checkbox("Confine To Bounds", &camera.confineToBounds);
	ImGui::DragFloat4("Bounds (x, y, w, h)", &camera.bounds.x, 1.0f);
	ImGui::TextDisabled("View Center: X %.1f Y %.1f", camera.viewCenter.x, camera.viewCenter.y);

	ImGui::Spacing();

	if (ImGui::TreeNode("Screen Shake"))
	{
		ImGui::DragFloat("Trauma", &camera.trauma, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Trauma Time", &camera.traumaTime, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Max Offset", &camera.maxOffset, 0.01f);
		ImGui::DragFloat("Max Angle", &camera.maxAngle, 0.01f);
		ImGui::DragFloat("Shake Amplitude", &camera.shakeAmplitude, 0.01f);

		ImGui::TreePop();
	}
}

void InspectorWindow::RenderSpriteAnimationComponent(GameContext& context, Component::SpriteAnimation& spriteAnimation,
                                                     entt::registry& registry, entt::entity entity)
{
	auto& animationSystem = context.GetSystemManager().GetSystem<System::AnimationSystem>();

	ImGui::Text("Current Animation: %s",
	            spriteAnimation.curAnimation.empty() ? "[None]" : spriteAnimation.curAnimation.c_str());

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Animations (%zu)", spriteAnimation.animations.size());

	// Editing the fields below writes straight into the component's animation map - the fields are
	// plain scalars so there's no need for a copy/apply step like Transform's quaternion conversion.
	for (auto& [name, animation] : spriteAnimation.animations)
	{
		ImGui::PushID(name.c_str());

		bool isCurrent = spriteAnimation.curAnimation == name;
		if (isCurrent)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
		}
		bool open = ImGui::TreeNodeEx(name.c_str(), isCurrent ? ImGuiTreeNodeFlags_DefaultOpen : 0, "%s%s",
		                              name.c_str(), isCurrent ? " (Current)" : "");
		if (isCurrent)
		{
			ImGui::PopStyleColor();
		}

		if (open)
		{
			int startFrame = static_cast<int>(animation.startFrame);
			if (ImGui::DragInt("Start Frame", &startFrame, 1.0f, 0, 10000))
			{
				animation.startFrame = static_cast<unsigned int>(startFrame);
			}

			int endFrame = static_cast<int>(animation.endFrame);
			if (ImGui::DragInt("End Frame", &endFrame, 1.0f, 0, 10000))
			{
				animation.endFrame = static_cast<unsigned int>(endFrame);
			}

			ImGui::DragFloat("Duration (s)", &animation.animationTime, 0.01f, 0.01f, 60.0f);
			ImGui::Checkbox("Loop", &animation.loop);

			if (ImGui::Button("Play"))
			{
				animationSystem.PlayAnimation(context, entity, name);
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	ImGui::Spacing();
	ImGui::Separator();

	if (ImGui::TreeNode("New Animation"))
	{
		ImGui::InputText("Name", m_newAnimationNameBuffer, sizeof(m_newAnimationNameBuffer));
		ImGui::DragInt("Start Frame##New", &m_newAnimationStartFrame, 1.0f, 0, 10000);
		ImGui::DragInt("End Frame##New", &m_newAnimationEndFrame, 1.0f, 0, 10000);
		ImGui::DragFloat("Duration (s)##New", &m_newAnimationTime, 0.01f, 0.01f, 60.0f);
		ImGui::Checkbox("Loop##New", &m_newAnimationLoop);

		bool nameEmpty = m_newAnimationNameBuffer[0] == '\0';
		bool nameTaken =
		    !nameEmpty && spriteAnimation.animations.find(m_newAnimationNameBuffer) != spriteAnimation.animations.end();

		ImGui::BeginDisabled(nameEmpty || nameTaken);
		if (ImGui::Button("Create Animation"))
		{
			Animation::SpriteAnimation newAnimation;
			newAnimation.startFrame    = static_cast<unsigned int>(m_newAnimationStartFrame);
			newAnimation.endFrame      = static_cast<unsigned int>(m_newAnimationEndFrame);
			newAnimation.animationTime = m_newAnimationTime;
			newAnimation.loop          = m_newAnimationLoop;

			animationSystem.AddAnimation(context, entity, m_newAnimationNameBuffer, newAnimation);

			m_newAnimationNameBuffer[0] = '\0';
			m_newAnimationStartFrame    = 0;
			m_newAnimationEndFrame      = 1;
			m_newAnimationTime          = 1.0f;
			m_newAnimationLoop          = true;
		}
		ImGui::EndDisabled();

		if (nameEmpty)
		{
			ImGui::TextDisabled("Enter a name to create an animation");
		}
		else if (nameTaken)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "An animation with this name already exists");
		}

		ImGui::TreePop();
	}
}

void InspectorWindow::RenderLevelComponent(GameContext& context, Component::Level& level, entt::registry& registry,
                                           entt::entity entity)
{
	// Sourced from the loaded level file (see World::Level::LoadLevelEntities) - not meaningful to edit here
	ImGui::Text("Index: %d", level.index);
	ImGui::Text("Iid: %s", level.Iid.c_str());
	ImGui::Text("Size: %d x %d px", level.width, level.height);
	ImGui::TextDisabled("Read-only - sourced from the loaded level file");
}

void InspectorWindow::RenderLevelEntityInstanceComponent(GameContext& context, Component::LevelEntityInstance& instance,
                                                         entt::registry& registry, entt::entity entity)
{
	// Sourced from the level viewer's loaded LDtk file (see World::Level::EntityMode::PREVIEW)
	ImGui::Text("Identifier: %s", instance.identifier.c_str());
	ImGui::Text("Size: %.0f x %.0f px", instance.size.x, instance.size.y);

	ImGui::Spacing();
	ImGui::TextUnformatted("Fields");
	ImGui::Indent();
	// Editable while the level viewer has the file open (it always does for a LevelEntityInstance, unless the file
	// couldn't be read for editing).
	LevelViewer& levelViewer = context.GetEditor().GetLevelViewer();
	if (levelViewer.IsActive() && levelViewer.GetDocument().HasEntity(instance.Iid))
	{
		if (registry.all_of<Component::ParticleEmitter>(entity))
		{
			if (ImGui::SmallButton("Replay"))
			{
				levelViewer.ReplayParticles(context, entity);
			}
			ImGui::SetItemTooltip("Clear the particles and fire the burst again");
		}
		if (ImGui::SmallButton("Copy Fields"))
		{
			levelViewer.CopyEntityFields(instance.Iid);
		}
		ImGui::SetItemTooltip("Copy this entity's field values (Ctrl+C), to paste onto another %s",
		                      instance.identifier.c_str());
		ImGui::SameLine();
		ImGui::BeginDisabled(!levelViewer.CanPasteEntityFields(instance.Iid));
		if (ImGui::SmallButton("Paste Fields"))
		{
			levelViewer.PasteEntityFields(context, instance.Iid);
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			if (!levelViewer.HasCopiedFields())
			{
				ImGui::SetTooltip("Copy the fields of another %s first", instance.identifier.c_str());
			}
			else if (levelViewer.CanPasteEntityFields(instance.Iid))
			{
				ImGui::SetTooltip("Replace every field with the copied %s's (Ctrl+V, one undo step)",
				                  levelViewer.GetCopiedFieldsIdentifier().c_str());
			}
			else
			{
				ImGui::SetTooltip("The copied fields are from a %s (or this same entity)",
				                  levelViewer.GetCopiedFieldsIdentifier().c_str());
			}
		}
		EntityFieldEditor::Render(context, levelViewer, instance.Iid);
		// Save / undo / redo / copy / paste while editing here, as in the Game Viewport and Level Viewer window.
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		{
			levelViewer.HandleShortcuts(context);
		}
		ImGui::Unindent();
		ImGui::TextDisabled("Saved with the level viewer's Save (Ctrl+S)");
		return;
	}
	for (const Wren::WrenItem& field : instance.fields)
	{
		ImGui::TextWrapped("%s: %s", field.identifier.c_str(), FormatFieldValue(field).c_str());
	}
	ImGui::Unindent();
	ImGui::TextDisabled("Read-only - sourced from the loaded level file");
}

void InspectorWindow::RenderWorldComponent(GameContext& context, Component::World& world, entt::registry& registry,
                                           entt::entity entity)
{
	const auto& worldMap = world.worldMap;

	// Sourced from the loaded LDtk world file (see World::Level::CreateWorldEntity) - not meaningful to edit here
	ImGui::Text("Iid: %s", worldMap.Iid.c_str());
	ImGui::Text("File: %s", worldMap.filePath.c_str());
	ImGui::Text("Levels: %zu", worldMap.levels.size());
	ImGui::TextDisabled("Read-only - sourced from the loaded level file");

	ImGui::Spacing();

	static const char* k_layerTypeNames[] = {"Entities", "Int Grid", "Auto Layer", "Tiles"};

	for (size_t i = 0; i < worldMap.levels.size(); i++)
	{
		const auto& level = worldMap.levels[i];

		ImGui::PushID((int)i);
		if (ImGui::TreeNode(level.identifier.c_str(), "[%zu] %s", i, level.identifier.c_str()))
		{
			ImGui::Text("Iid: %s", level.Iid.c_str());
			ImGui::Text("World Position: (%d, %d)", level.worldX, level.worldY);
			ImGui::Text("Size: %d x %d px", level.pxWid, level.pxHei);
			ImGui::Text("Layers: %zu", level.layers.size());

			if (!level.neighbours.empty() && ImGui::TreeNode("Neighbours"))
			{
				for (const auto& neighbour : level.neighbours)
				{
					ImGui::BulletText("%s (%s)", neighbour.levelIid.c_str(), neighbour.dir.c_str());
				}
				ImGui::TreePop();
			}

			if (!level.tags.empty())
			{
				std::string tagsText;
				for (size_t t = 0; t < level.tags.size(); t++)
				{
					if (t > 0)
					{
						tagsText += ", ";
					}
					tagsText += level.tags[t];
				}
				ImGui::Text("Tags: %s", tagsText.c_str());
			}

			if (!level.layers.empty() && ImGui::TreeNode("Layers"))
			{
				for (const auto& layer : level.layers)
				{
					int typeIndex        = static_cast<int>(layer.type);
					const char* typeName = (typeIndex >= 0 && typeIndex < IM_ARRAYSIZE(k_layerTypeNames))
					                           ? k_layerTypeNames[typeIndex]
					                           : "Unknown";
					ImGui::BulletText("%s (%s)", layer.identifier.c_str(), typeName);
				}
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}

void InspectorWindow::RenderTileMapComponent(GameContext& context, Component::TileMap& tileMap,
                                             entt::registry& registry, entt::entity entity)
{
	if (tileMap.texture)
	{
		ImGui::Text("Texture: Loaded");
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Texture: None");
	}
	ImGui::Text("Grid: %d x %d cells (%d px tiles)", tileMap.width, tileMap.height, tileMap.tileSize);
	ImGui::Text("Visible Tiles: %zu", tileMap.gridTiles.size());
	ImGui::TextDisabled("The dense collision grid is baked into physics at load and isn't re-generated here");

	static const char* k_renderLayerNames[] = {"Background Far",     "Background Mid", "Entities",
	                                           "Background Overlay", "Foreground",     "UI"};
	int layerIndex                          = static_cast<int>(tileMap.layer);
	if (ImGui::Combo("Render Layer", &layerIndex, k_renderLayerNames, IM_ARRAYSIZE(k_renderLayerNames)))
	{
		tileMap.layer = static_cast<World::RenderLayer>(layerIndex);
	}
	ImGui::DragFloat("Order In Layer", &tileMap.orderInLayer, 1.0f, -1000.0f, 1000.0f);

	ImGui::Spacing();
	ImGui::Separator();

	const int cellCount = tileMap.width * tileMap.height;
	if (tileMap.width <= 0 || tileMap.height <= 0 || tileMap.tileSize <= 0)
	{
		ImGui::TextDisabled("No grid data");
		return;
	}
	if (cellCount > 4096)
	{
		ImGui::TextDisabled("Grid too large to edit here (%d cells)", cellCount);
		return;
	}

	// gridTiles only holds the tiles that are actually drawn (sparse), keyed by tile-local pixel position -
	// rebuild a cell -> gridTiles index lookup each frame rather than caching it, since edits below can
	// add/remove entries.
	std::unordered_map<int, int> cellToTileIndex;
	for (int i = 0; i < (int)tileMap.gridTiles.size(); i++)
	{
		const World::TileMap::GridTile& tile = tileMap.gridTiles[i];
		int col                              = (int)std::round(tile.position.x / tileMap.tileSize);
		int row                              = (int)std::round(tile.position.y / tileMap.tileSize);
		if (col >= 0 && col < tileMap.width && row >= 0 && row < tileMap.height)
		{
			cellToTileIndex[row * tileMap.width + col] = i;
		}
	}

	ImGui::Text("Click a cell to select it (blue = tile present)");
	ImGui::BeginChild("TileGrid", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

	const float cellButtonSize = 18.0f;
	for (int row = 0; row < tileMap.height; row++)
	{
		ImGui::PushID(row);
		for (int col = 0; col < tileMap.width; col++)
		{
			int cellIndex = row * tileMap.width + col;
			bool hasTile  = cellToTileIndex.find(cellIndex) != cellToTileIndex.end();
			bool selected = m_selectedTileMapCell == cellIndex;

			ImVec4 color = selected ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f)
			                        : (hasTile ? ImVec4(0.3f, 0.6f, 0.9f, 1.0f) : ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

			ImGui::PushID(col);
			ImGui::PushStyleColor(ImGuiCol_Button, color);
			if (ImGui::Button("##cell", ImVec2(cellButtonSize, cellButtonSize)))
			{
				m_selectedTileMapCell = cellIndex;
			}
			ImGui::PopStyleColor();
			ImGui::PopID();

			if (col < tileMap.width - 1)
			{
				ImGui::SameLine(0.0f, 2.0f);
			}
		}
		ImGui::PopID();
	}

	ImGui::EndChild();

	ImGui::Spacing();
	ImGui::Separator();

	if (m_selectedTileMapCell < 0 || m_selectedTileMapCell >= cellCount)
	{
		ImGui::TextDisabled("No cell selected");
		return;
	}

	int selectedRow = m_selectedTileMapCell / tileMap.width;
	int selectedCol = m_selectedTileMapCell % tileMap.width;
	ImGui::Text("Selected Cell: (%d, %d)", selectedCol, selectedRow);

	auto it = cellToTileIndex.find(m_selectedTileMapCell);
	if (it == cellToTileIndex.end())
	{
		ImGui::TextDisabled("Empty cell");
		if (ImGui::Button("Add Tile"))
		{
			World::TileMap::GridTile newTile;
			newTile.position       = glm::vec2(selectedCol * tileMap.tileSize, selectedRow * tileMap.tileSize);
			newTile.sourcePosition = glm::vec2(0.0f, 0.0f);
			newTile.flipBit        = Renderer::FlipBit::NONE;
			tileMap.gridTiles.push_back(newTile);
			tileMap.chunksBuilt = false;
		}
		return;
	}

	World::TileMap::GridTile& tile = tileMap.gridTiles[it->second];
	bool changed                   = false;

	int atlasCol = (int)std::round(tile.sourcePosition.x / tileMap.tileSize);
	int atlasRow = (int)std::round(tile.sourcePosition.y / tileMap.tileSize);
	if (ImGui::DragInt("Atlas Column", &atlasCol, 1.0f, 0, 1000))
	{
		changed = true;
	}
	if (ImGui::DragInt("Atlas Row", &atlasRow, 1.0f, 0, 1000))
	{
		changed = true;
	}
	if (changed)
	{
		tile.sourcePosition = glm::vec2(atlasCol * tileMap.tileSize, atlasRow * tileMap.tileSize);
	}

	static const char* k_flipNames[] = {"None", "Horizontal", "Vertical", "Both"};
	int flipIndex                    = static_cast<int>(tile.flipBit);
	if (ImGui::Combo("Flip", &flipIndex, k_flipNames, IM_ARRAYSIZE(k_flipNames)))
	{
		tile.flipBit = static_cast<Renderer::FlipBit>(flipIndex);
		changed      = true;
	}

	if (ImGui::Button("Clear Tile"))
	{
		tileMap.gridTiles.erase(tileMap.gridTiles.begin() + it->second);
		changed = true;
	}

	if (changed)
	{
		tileMap.chunksBuilt = false;
	}
}

void InspectorWindow::RenderSpriteComponent(GameContext& context, Component::Sprite& sprite, entt::registry& registry,
                                            entt::entity entity)
{
	// Preview button
	if (ImGui::Button("Preview Sprite"))
	{
		if (m_previewWindow)
		{
			PreviewSprite(m_previewWindow, &sprite, "Sprite Component");
		}
	}

	// Texture info
	if (sprite.texture)
	{
		ImGui::Text("Texture: Loaded");
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Texture: None");
	}

	ImGui::Spacing();

	// Color tint
	RenderColor("Color Tint", sprite.color);

	// Offset
	RenderVec2("Offset", sprite.offset);

	ImGui::Spacing();

	// Sprite sheet properties
	ImGui::Text("Sprite Sheet:");
	ImGui::DragInt("Columns", &sprite.columns, 1.0f, 1, 100);
	ImGui::DragInt("Rows", &sprite.rows, 1.0f, 1, 100);
	ImGui::DragInt("Index", &sprite.index, 1.0f, 0, sprite.columns * sprite.rows - 1);

	ImGui::Spacing();

	// Flipped
	static const char* k_spriteFlipNames[] = {"None", "Horizontal", "Vertical", "Both"};
	int spriteFlipIndex                    = static_cast<int>(sprite.flipped);
	if (ImGui::Combo("Flipped", &spriteFlipIndex, k_spriteFlipNames, IM_ARRAYSIZE(k_spriteFlipNames)))
	{
		sprite.flipped = static_cast<Renderer::FlipBit>(spriteFlipIndex);
	}

	// Render layer + order
	static const char* k_renderLayerNames[] = {"Background Far",     "Background Mid", "Entities",
	                                           "Background Overlay", "Foreground",     "UI"};
	int layerIndex                          = static_cast<int>(sprite.layer);
	if (ImGui::Combo("Render Layer", &layerIndex, k_renderLayerNames, IM_ARRAYSIZE(k_renderLayerNames)))
	{
		sprite.layer = static_cast<World::RenderLayer>(layerIndex);
	}
	ImGui::DragFloat("Order In Layer", &sprite.orderInLayer, 1.0f, -1000.0f, 1000.0f);
	if (sprite.layer == World::RenderLayer::Entities)
	{
		ImGui::TextDisabled("(world Y is added automatically for y-sort on this layer)");
	}

	ImGui::Spacing();

	// Preview calculated values
	if (ImGui::TreeNode("Calculated Values"))
	{
		int totalFrames = sprite.columns * sprite.rows;
		ImGui::Text("Total Frames: %d", totalFrames);

		if (totalFrames > 0)
		{
			int row = sprite.index / sprite.columns;
			int col = sprite.index % sprite.columns;
			ImGui::Text("Current Frame: Row %d, Col %d", row, col);
		}

		ImGui::TreePop();
	}
}

void InspectorWindow::RenderParticleEmitterComponent(GameContext& context, Component::ParticleEmitter& emitter,
                                                      entt::registry& registry, entt::entity entity)
{
	// Texture info - status only, same "no asset picker in this build" convention as RenderSpriteComponent's
	// own texture field above.
	if (emitter.texture)
	{
		ImGui::Text("Texture: Loaded");
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Texture: None");
	}

	int aliveCount = 0;
	for (const auto& particle : emitter.particles)
	{
		if (particle.alive)
		{
			++aliveCount;
		}
	}
	ImGui::Text("Alive: %d / %d", aliveCount, emitter.maxParticles);

	ImGui::Spacing();

	// Sprite sheet properties - same fields/ranges as RenderSpriteComponent's own, since flipbook animation
	// (driving index from a particle's own age) reuses that exact math - see ParticleEmitter's own comment.
	ImGui::Text("Sprite Sheet:");
	ImGui::DragInt("Columns", &emitter.columns, 1.0f, 1, 100);
	ImGui::DragInt("Rows", &emitter.rows, 1.0f, 1, 100);
	{
		const char* modes[] = {"None", "Once", "Loop", "Lifetime"};
		int mode            = static_cast<int>(emitter.animationMode);
		if (ImGui::Combo("Animation", &mode, modes, IM_ARRAYSIZE(modes)))
		{
			emitter.animationMode = static_cast<Component::ParticleAnimationMode>(mode);
		}
		int startFrame = static_cast<int>(emitter.animation.startFrame);
		int endFrame   = static_cast<int>(emitter.animation.endFrame);
		if (ImGui::DragInt("Start Frame", &startFrame, 1.0f, 0, 10000))
		{
			emitter.animation.startFrame = static_cast<unsigned int>(std::max(startFrame, 0));
		}
		if (ImGui::DragInt("End Frame (excl.)", &endFrame, 1.0f, 0, 10000))
		{
			emitter.animation.endFrame = static_cast<unsigned int>(std::max(endFrame, 0));
		}
		ImGui::DragFloat("Animation Time", &emitter.animation.animationTime, 0.01f, 0.01f, 60.0f, "%.2fs");
	}

	ImGui::Spacing();

	ImGui::Text("Emission:");
	ImGui::Checkbox("Looping", &emitter.looping);
	ImGui::DragFloat("Emission Rate", &emitter.emissionRate, 0.5f, 0.0f, 1000.0f, "%.1f/sec");
	if (ImGui::DragInt("Burst Count", &emitter.burstCount, 1.0f, 0, 10000))
	{
		// A new burst count should fire again - matches wren_ParticleEmitterSetBurstCount's own behavior for
		// scripts (WrenGameObjectComponents.cpp), so editing this field live behaves the same way.
		emitter.hasBurst = false;
	}
	ImGui::DragFloat("Spawn Radius", &emitter.spawnRadius, 0.5f, 0.0f, 1000.0f);
	ImGui::DragInt("Max Particles", &emitter.maxParticles, 1.0f, 1, 10000);

	ImGui::Spacing();

	ImGui::Text("Motion:");
	RenderVec2("Velocity Min", emitter.velocityMin);
	RenderVec2("Velocity Max", emitter.velocityMax);
	RenderVec2("Acceleration", emitter.acceleration);
	ImGui::DragFloat("Rotation Speed Min", &emitter.rotationSpeedMin, 0.01f, -20.0f, 20.0f, "%.2f rad/s");
	ImGui::DragFloat("Rotation Speed Max", &emitter.rotationSpeedMax, 0.01f, -20.0f, 20.0f, "%.2f rad/s");

	ImGui::Spacing();

	ImGui::Text("Lifetime:");
	ImGui::DragFloat("Lifetime Min", &emitter.lifetimeMin, 0.05f, 0.01f, 60.0f, "%.2fs");
	ImGui::DragFloat("Lifetime Max", &emitter.lifetimeMax, 0.05f, 0.01f, 60.0f, "%.2fs");

	ImGui::Spacing();

	ImGui::Text("Appearance Over Lifetime:");
	RenderColor("Start Color", emitter.startColor);
	RenderColor("End Color", emitter.endColor);
	ImGui::DragFloat("Start Scale", &emitter.startScale, 0.05f, 0.0f, 100.0f);
	ImGui::DragFloat("End Scale", &emitter.endScale, 0.05f, 0.0f, 100.0f);
	ImGui::Checkbox("Additive Blend", &emitter.additive);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(
		    "Order-independent - the natural default for sparks/fire/glow. Off gives normal alpha "
		    "blending, better for smoke/dust (see SpriteBatchRenderer's own additive-blend comment).");
	}

	ImGui::Spacing();

	// Render layer + order - same fields/ranges as RenderSpriteComponent's own.
	static const char* k_renderLayerNames[] = {"Background Far",     "Background Mid", "Entities",
	                                           "Background Overlay", "Foreground",     "UI"};
	int layerIndex                          = static_cast<int>(emitter.layer);
	if (ImGui::Combo("Render Layer", &layerIndex, k_renderLayerNames, IM_ARRAYSIZE(k_renderLayerNames)))
	{
		emitter.layer = static_cast<World::RenderLayer>(layerIndex);
	}
	ImGui::DragFloat("Order In Layer", &emitter.orderInLayer, 1.0f, -1000.0f, 1000.0f);
}

void InspectorWindow::RenderShaderComponent(GameContext& context, Component::Shader& shader, entt::registry& registry,
                                            entt::entity entity)
{
	// Preview button
	if (ImGui::Button("Preview Shader"))
	{
		if (m_previewWindow)
		{
			PreviewShader(m_previewWindow, &shader, "Shader Component");
		}
	}

	ImGui::Separator();

	ImGui::Text("Shader ID: %d", shader.shader->shader.idx);
	ImGui::Separator();

	// Float uniforms
	if (ImGui::TreeNode("Float Uniforms"))
	{
		if (shader.floatUniforms.empty())
		{
			ImGui::TextDisabled("No float uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.floatUniforms)
			{
				ImGui::PushID(name.c_str());
				ImGui::DragFloat(name.c_str(), &value, 0.01f);
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Int uniforms
	if (ImGui::TreeNode("Int Uniforms"))
	{
		if (shader.intUniforms.empty())
		{
			ImGui::TextDisabled("No int uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.intUniforms)
			{
				ImGui::PushID(name.c_str());
				ImGui::DragInt(name.c_str(), &value);
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Vec2 uniforms
	if (ImGui::TreeNode("Vector2 Uniforms"))
	{
		if (shader.vec2Uniforms.empty())
		{
			ImGui::TextDisabled("No vec2 uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.vec2Uniforms)
			{
				ImGui::PushID(name.c_str());
				float components[2] = {value.x, value.y};
				if (ImGui::DragFloat2(name.c_str(), components, 0.01f))
				{
					value.x = components[0];
					value.y = components[1];
				}
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Vec3 uniforms
	if (ImGui::TreeNode("Vector3 Uniforms"))
	{
		if (shader.vec3Uniforms.empty())
		{
			ImGui::TextDisabled("No vec3 uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.vec3Uniforms)
			{
				ImGui::PushID(name.c_str());
				float components[3] = {value.x, value.y, value.z};
				if (ImGui::DragFloat3(name.c_str(), components, 0.01f))
				{
					value.x = components[0];
					value.y = components[1];
					value.z = components[2];
				}
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Vec4 uniforms
	if (ImGui::TreeNode("Vector4 Uniforms"))
	{
		if (shader.vec4Uniforms.empty())
		{
			ImGui::TextDisabled("No vec4 uniforms");
		}
		else
		{
			for (auto& [name, value] : shader.vec4Uniforms)
			{
				ImGui::PushID(name.c_str());
				float components[4] = {value.x, value.y, value.z, value.w};
				if (ImGui::DragFloat4(name.c_str(), components, 0.01f))
				{
					value.x = components[0];
					value.y = components[1];
					value.z = components[2];
					value.w = components[3];
				}
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}

	// Matrix uniforms (read-only for now)
	if (ImGui::TreeNode("Matrix Uniforms"))
	{
		if (shader.matrixUniforms.empty())
		{
			ImGui::TextDisabled("No matrix uniforms");
		}
	}
}
}  // namespace Struktur::Debug
