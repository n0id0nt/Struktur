#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>
#include <vector>

#include "Engine/Renderer/RenderTypes.h"
#include "Engine/Renderer/TileChunk.h"
#include "Engine/Util/Color.h"
#include "Engine/Util/MathUtil.h"
#include "Engine/World/RenderLayer.h"
#include "glm/glm.hpp"

namespace Struktur
{
class GameContext;

namespace Component
{
struct Shader;
}

namespace Renderer
{
struct DrawItem
{
	uint64_t sortKey;
	TextureHandle texture;
	// Resolved by the caller (see System::ShaderSystem::ResolveProgram) and handed to SubmitSprite/SubmitChunk
	// directly - WorldRenderer has no business knowing about entt::entity, it only needs the resulting bgfx
	// handle to sort/batch by. Also folded into sortKey (see PackSortKey), so the two must always come from the
	// same resolve call.
	bgfx::ProgramHandle program;
	// Resolved by the caller alongside program (nullptr if the entity has no Component::Shader). Applied via
	// ShaderSystem::ApplyUniforms right before this item's own run is actually submitted (see Flush()/FlushRun)
	// - not any earlier, since bgfx's per-draw uniform state is only valid for the very next submit on this
	// encoder, and setting it earlier for one item while a second shaded item's values are staged before either
	// has actually been submitted is exactly what used to crash (bgfx flags re-setting an already-set uniform).
	const Component::Shader* shader = nullptr;
	Util::Math::Rect sourceRec;
	Util::Math::Rect destRec;
	glm::vec2 origin;
	float rotation;
	Util::Color tint;
	// Additive (BGFX_STATE_BLEND_FUNC(SRC_ALPHA, ONE)) instead of the default alpha blend - see
	// Component::ParticleEmitter::additive's own comment for why particles want this. Folded into the run-
	// boundary check in Flush() (like texture/program) so additive and normal-blended items never merge into
	// one draw call sharing one blend state.
	bool additive           = false;
	// Set only for chunk items (see SubmitChunk) - Flush() submits these directly from their own cached
	// static buffers instead of folding them into a transient sprite batch.
	const TileChunk* chunk = nullptr;
};

struct CullBounds
{
	float minX, minY, maxX, maxY;
};

// Owns the per-frame draw list for everything in the game world - individual sprites/tiles (SubmitSprite) and
// cached tilemap chunk meshes (SubmitChunk) alike - so both funnel through the same cull, sort, and flush,
// even though tiles-as-a-chunk and a moving sprite end up drawn very differently once Flush() gets to them.
class WorldRenderer
{
public:
	WorldRenderer();

	// Explicit rather than in the destructor - must run before GraphicsDevice::Shutdown() (see UIRenderer::
	// Shutdown's own comment), since bgfx::destroy() is invalid once bgfx itself has shut down.
	void Shutdown();

	static CullBounds ComputeCullBounds(GameContext& context);
	static uint64_t PackSortKey(World::RenderLayer layer, float orderInLayer, unsigned int textureId,
	                            uint16_t programId);

	void Clear();
	// program/shader are resolved by the caller (System::ShaderSystem::ResolveProgram, Component::Shader lookup)
	// before calling in - WorldRenderer only needs the resulting bgfx handle to sort/batch by and the resolved
	// component to apply uniforms from later, not the entt::entity or GameContext that produced them. This also
	// means SubmitChunk (below) can be handed a real per-tilemap program/shader instead of being hardcoded to
	// whatever entt::null happens to resolve to.
	void SubmitSprite(World::RenderLayer layer, float orderInLayer, bgfx::ProgramHandle program,
	                  const Component::Shader* shader, const TextureHandle& texture, const Util::Math::Rect& sourceRec,
	                  const Util::Math::Rect& destRec, const glm::vec2& origin, float rotation, const Util::Color& tint,
	                  bool additive, const CullBounds& cullBounds);
	void SubmitChunk(World::RenderLayer layer, float orderInLayer, bgfx::ProgramHandle program,
	                 const Component::Shader* shader, const TileChunk& chunk, const TextureHandle& texture,
	                 const CullBounds& cullBounds);
	void Flush(GameContext& context);

private:
	// Builds a transient vertex/index buffer for m_drawItems[runStart, runEnd) - all sharing runTexture/runProgram
	// - and submits it in a single draw call. See Flush() for how runs are formed. Takes GameContext& (unlike
	// SubmitSprite/SubmitChunk) purely to reach ShaderSystem::ApplyUniforms right before this run's own submit -
	// see DrawItem::shader.
	void FlushRun(GameContext& context, size_t runStart, size_t runEnd, const bgfx::VertexLayout& quadLayout,
	              bgfx::UniformHandle texColorSampler, bool runAdditive, const TextureHandle& runTexture,
	              bgfx::ProgramHandle runProgram);

	// FlushRun's transient index buffer uses 16-bit indices (uint16_t ibase) - a run any longer than this would
	// wrap the index space and silently corrupt geometry instead of erroring, so Flush() forces a run boundary
	// at this size even when texture/program didn't change (see Flush()'s run-forming loop).
	static constexpr size_t kMaxQuadsPerRun = 16384;

	std::vector<DrawItem> m_drawItems;
	// Lazily created on first Flush() (like the rest of this class, WorldRenderer exists before bgfx does - see
	// the constructor) rather than in an Initialise() - held as a member instead of Flush()'s old function-local
	// static purely so Shutdown() above has something to destroy.
	bgfx::UniformHandle m_texColorSampler = BGFX_INVALID_HANDLE;
};
}  // namespace Renderer
}  // namespace Struktur
