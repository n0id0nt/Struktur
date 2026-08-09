#include "SpriteRenderSystem.h"

#include "Debug/Assertions.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/Shader.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/ShaderSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/WorldRenderer.h"
#include "Engine/Util/MathUtil.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "raylib.h"
#include "raymath.h"

#if !defined(PLATFORM_WEB)
	#include "Engine/Renderer/TileChunkBuilder.h"
#endif

void Struktur::System::SpriteRenderSystem::Update(GameContext& context)
{
	entt::registry& registry               = context.GetRegistry();
	GameResource::Camera& camera           = context.GetCamera();
	Renderer::WorldRenderer& worldRenderer = context.GetWorldRenderer();
	TransformSystem& transformSystem       = context.GetSystemManager().GetSystem<TransformSystem>();

#if defined(PLATFORM_WEB)
	::BeginMode2D(camera.GetRaylibCamera());
#endif
	{
		Renderer::CullBounds cullBounds = Renderer::WorldRenderer::ComputeCullBounds(context);
		worldRenderer.Clear();

		{
			auto view = registry.view<Component::TileMap, Component::Transform>(entt::exclude<Inactive>);
			for (const auto& [entity, tileMap, transform] : view.each())
			{
				Resource::TextureResource* texture = tileMap.texture.Get();
				if (!texture)
				{
					continue;
				}

				if (!texture->IsGpuReady())
				{
					texture->LoadToGpu(context);
				}

				glm::vec3 worldPosition = transformSystem.GetWorldPosition(context, entity);

#if !defined(PLATFORM_WEB)
				// Tilemaps are treated as static once placed: chunks are built once (from world-space tile
				// positions baked in up front) and cached on the component, rather than every tile being
				// walked and resubmitted every frame - see the chunking design discussion for why.
				if (!tileMap.chunksBuilt)
				{
					tileMap.chunks     = Renderer::BuildTileChunks(tileMap.gridTiles, tileMap.tileSize,
					                                               glm::vec2(worldPosition.x, worldPosition.y),
					                                               texture->GetWidth(), texture->GetHeight());
					tileMap.chunksBuilt = true;
				}

				for (const Renderer::TileChunk& chunk : tileMap.chunks)
				{
					worldRenderer.SubmitChunk(tileMap.layer, tileMap.orderInLayer, chunk, texture->GetHandle(),
					                          cullBounds);
				}
#else
				for (auto& gridTile : tileMap.gridTiles)
				{
					Util::Math::Rect sourceRec{gridTile.sourcePosition.x, gridTile.sourcePosition.y,
					                      (float)tileMap.tileSize, (float)tileMap.tileSize};
					switch (gridTile.flipBit)
					{
						case GameResource::TileMap::FlipBit::BOTH:
							sourceRec.width *= -1;
							sourceRec.height *= -1;
							break;
						case GameResource::TileMap::FlipBit::HORIZONTAL:
							sourceRec.width *= -1;
							break;
						case GameResource::TileMap::FlipBit::VERTIAL:
							sourceRec.height *= -1;
							break;
					}
					// this stops you from seeing a little bit of the neighbouring sprite
					sourceRec.x += 0.0001f;
					sourceRec.y += 0.0001f;
					sourceRec.width -= 0.0002f;
					sourceRec.height -= 0.0002f;
					// TODO - Move this to a helper function so this line is much more consise
					Util::Math::Rect destRec{gridTile.position.x + ::round(worldPosition.x * 2) / 2,
					                       gridTile.position.y + ::round(worldPosition.y * 2) / 2,
					                       (float)tileMap.tileSize, (float)tileMap.tileSize};

					worldRenderer.Submit(tileMap.layer, tileMap.orderInLayer, entity, texture->GetHandle(), sourceRec,
					                     destRec, glm::vec2{0, 0}, 0.0f, Util::Math::ColorWhite, cullBounds);
				}
#endif
			}
		}
		{
			auto view = registry.view<Component::Sprite, Component::Transform>(entt::exclude<Inactive>);
			for (const auto& [entity, sprite, transform] : view.each())
			{
				Resource::TextureResource* texture = sprite.texture.Get();
				if (!texture)
				{
					continue;
				}

				if (!texture->IsGpuReady())
				{
					texture->LoadToGpu(context);
				}

				glm::vec3 worldPosition = transformSystem.GetWorldPosition(context, entity);
				glm::quat worldRotation = transformSystem.GetWorldRotation(context, entity);
				glm::vec3 worldScale    = transformSystem.GetWorldScale(context, entity);

				int imageWidth  = texture->GetWidth();
				int imageHeight = texture->GetHeight();
				float angleZ    = Struktur::Util::Math::AngleZFromQuat(worldRotation);
				int index       = sprite.index;

				ASSERT_MSG(sprite.columns > 0, "Sprite needs to have at least one column");
				ASSERT_MSG(sprite.rows > 0, "Sprite needs to have at least one row");

				glm::vec2 size = glm::vec2(imageWidth / sprite.columns, imageHeight / sprite.rows);
				int x          = (index % sprite.columns) * size.x;
				int y          = std::floor(index / sprite.columns) * size.y;

				Util::Math::Rect sourceRec{(float)x, (float)y, size.x, size.y};
				if (sprite.flipped)
				{
					sourceRec.width *= -1;
				}

				// This stops a little of the next sprite in the sprite sheet from showing due to rounding error in
				// the GPU
				sourceRec.x += 0.0001f;
				sourceRec.y += 0.0001f;
				sourceRec.width -= 0.0002f;
				sourceRec.height -= 0.0002f;

				Util::Math::Rect destRec{::round(worldPosition.x * 2) / 2, ::round(worldPosition.y * 2) / 2,
				                       size.x * worldScale.x, size.y * worldScale.x};
				glm::vec2 offset{sprite.offset.x, sprite.offset.y};

				float orderInLayer = sprite.orderInLayer;
				if (sprite.layer == GameResource::RenderLayer::Entities)
				{
					// y-sort: interleave with whatever background layers sit immediately above/below Entities
					orderInLayer += worldPosition.y;
				}

				worldRenderer.Submit(sprite.layer, orderInLayer, entity, texture->GetHandle(), sourceRec, destRec,
				                     offset, glm::degrees(angleZ), sprite.color, cullBounds);
			}
		}

		worldRenderer.Flush(context);
	}
#if defined(PLATFORM_WEB)
	::EndMode2D();
#endif
}
