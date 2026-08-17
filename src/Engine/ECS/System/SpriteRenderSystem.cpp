#include "SpriteRenderSystem.h"

#include "Debug/Assertions.h"
#include "Engine/ECS/Component/Shader.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/ShaderSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Renderer/TileChunkBuilder.h"
#include "Engine/Renderer/WorldRenderer.h"
#include "Engine/Util/MathUtil.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."

void Struktur::System::SpriteRenderSystem::Update(GameContext& context)
{
	entt::registry& registry               = context.GetRegistry();
	Renderer::WorldRenderer& worldRenderer = context.GetWorldRenderer();
	TransformSystem& transformSystem       = context.GetSystemManager().GetSystem<TransformSystem>();
	ShaderSystem& shaderSystem             = context.GetSystemManager().GetSystem<ShaderSystem>();
	// Resolved here rather than inside WorldRenderer - it only needs the resulting bgfx handle to sort/batch
	// by, not the entt::entity or Component::Shader that produced it (see WorldRenderer::SubmitSprite).
	bgfx::ProgramHandle defaultProgram = context.GetGraphicsDevice().GetDefaultSpriteProgram();

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

				// Tilemaps are treated as static once placed: chunks are built once (from world-space tile
				// positions baked in up front) and cached on the component, rather than every tile being
				// walked and resubmitted every frame - see the chunking design discussion for why.
				if (!tileMap.chunksBuilt)
				{
					tileMap.chunks      = Renderer::BuildTileChunks(tileMap.gridTiles, tileMap.tileSize,
					                                                glm::vec2(worldPosition.x, worldPosition.y),
					                                                texture->GetWidth(), texture->GetHeight());
					tileMap.chunksBuilt = true;
				}

				// Resolved from the tilemap's own entity (not entt::null) so a Component::Shader on the tilemap
				// actually takes effect - all of a tilemap's chunks share this one resolve since they're all the
				// same entity's shader, just different cached mesh ranges.
				bgfx::ProgramHandle program     = shaderSystem.ResolveProgram(context, entity, defaultProgram);
				const Component::Shader* shader = registry.try_get<Component::Shader>(entity);
				for (const Renderer::TileChunk& chunk : tileMap.chunks)
				{
					worldRenderer.SubmitChunk(tileMap.layer, tileMap.orderInLayer, program, shader, chunk,
					                          texture->GetHandle(), cullBounds);
				}
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
				if (sprite.layer == World::RenderLayer::Entities)
				{
					// y-sort: interleave with whatever background layers sit immediately above/below Entities
					orderInLayer += worldPosition.y;
				}

				bgfx::ProgramHandle program     = shaderSystem.ResolveProgram(context, entity, defaultProgram);
				const Component::Shader* shader = registry.try_get<Component::Shader>(entity);
				worldRenderer.SubmitSprite(sprite.layer, orderInLayer, program, shader, texture->GetHandle(), sourceRec,
				                           destRec, offset, glm::degrees(angleZ), sprite.color, cullBounds);
			}
		}

		worldRenderer.Flush(context);
	}
}
