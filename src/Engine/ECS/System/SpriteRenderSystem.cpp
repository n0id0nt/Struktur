#include "SpriteRenderSystem.h"

#include "Debug/Assertions.h"
#include "Engine/ECS/Component/ParticleEmitter.h"
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
				                           destRec, offset, glm::degrees(angleZ), sprite.color, false, cullBounds);
			}
		}

		// Particles - System::ParticleSystem (an update system, so it's already run this frame) owns spawning
		// and simulating each Component::ParticleEmitter's pool; this just turns every currently-alive particle
		// into a sprite submission, reusing the exact same texture-atlas frame math as Component::Sprite above
		// (see ParticleEmitter's own comment for why rendering lives here instead of in ParticleSystem).
		{
			// No Component::Transform needed here (unlike the sprite loop above) - particle.position is already
			// absolute world space, computed once at spawn time from the emitter's world position and integrated
			// there ever since by System::ParticleSystem (which does need Transform, for that spawn-time read).
			auto view = registry.view<Component::ParticleEmitter>(entt::exclude<Inactive>);
			for (const auto& [entity, emitter] : view.each())
			{
				Resource::TextureResource* texture = emitter.texture.Get();
				if (!texture)
				{
					continue;
				}

				if (!texture->IsGpuReady())
				{
					texture->LoadToGpu(context);
				}

				int imageWidth  = texture->GetWidth();
				int imageHeight = texture->GetHeight();

				ASSERT_MSG(emitter.columns > 0, "ParticleEmitter needs to have at least one column");
				ASSERT_MSG(emitter.rows > 0, "ParticleEmitter needs to have at least one row");
				glm::vec2 frameSize = glm::vec2(imageWidth / emitter.columns, imageHeight / emitter.rows);

				// Frame index 0 for every particle - flipbook animation (driving this from each particle's own
				// age) is a documented follow-up, not part of this build (see ParticleEmitter's own comment).
				Util::Math::Rect sourceRec{0.0f, 0.0f, frameSize.x, frameSize.y};

				bgfx::ProgramHandle program     = shaderSystem.ResolveProgram(context, entity, defaultProgram);
				const Component::Shader* shader = registry.try_get<Component::Shader>(entity);

				for (const Component::Particle& particle : emitter.particles)
				{
					if (!particle.alive)
					{
						continue;
					}

					float t              = particle.age / particle.lifetime;
					float scale          = Util::Math::Lerp(emitter.startScale, emitter.endScale, t);
					Util::Color color    = Util::Color(glm::mix(glm::vec4(emitter.startColor), glm::vec4(emitter.endColor), t));
					Util::Math::Rect destRec{::round(particle.position.x * 2) / 2, ::round(particle.position.y * 2) / 2,
					                         frameSize.x * scale, frameSize.y * scale};
					glm::vec2 origin{destRec.width * 0.5f, destRec.height * 0.5f};  // rotate/scale about center

					worldRenderer.SubmitSprite(emitter.layer, emitter.orderInLayer, program, shader,
					                           texture->GetHandle(), sourceRec, destRec, origin,
					                           glm::degrees(particle.rotation), color, emitter.additive, cullBounds);
				}
			}
		}

		worldRenderer.Flush(context);
	}
}
