#pragma once

#include <bgfx/bgfx.h>
#include <entt/entt.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/World/AssetManifest.h"
#include "Engine/Resource/TextureResource.h"
#include "Engine/World/Camera.h"
#include "Engine/World/World.h"
#include "Engine/World/Placeholder.h"
#include "Engine/World/SpriteDescriptor.h"

namespace Struktur
{
class GameContext;

namespace Renderer
{
// Draws a World::World with bgfx, behind the ride HUD:
//
//   RideSkyViewId     low-res framebuffer, orthographic: sky gradient, stars, mountain bands, sun/moon, clouds
//   RideSceneViewId   same framebuffer, perspective: terrain, water, road, instanced props, actors, banners, weather
//   RideOverlayViewId same framebuffer, orthographic: the rider sprite and its shadow (pixel-matched to the scene)
//   RidePostViewId    the world target at native size: nearest/linear upscale, optional dither + CRT scanlines
//
// The HUD (UIViewId) runs after these (GraphicsDevice's view order) at native resolution. Chunk meshes arrive from
// the world's ChunkStreamer and become static vertex/index/instance buffers; released chunks are destroyed. The
// road's elevation goes up every frame as a 1-D float texture the vertex shaders read.
class WorldRenderer
{
public:
	WorldRenderer() = default;
	~WorldRenderer();

	WorldRenderer(const WorldRenderer&)            = delete;
	WorldRenderer& operator=(const WorldRenderer&) = delete;

	// Creates GPU resources and loads every manifest role (real file or placeholder). Needs bgfx initialised.
	void Initialise(GameContext& context, const World::AssetManifest& manifest,
	                const World::SpriteDescriptor& descriptor);
	void Shutdown();
	bool IsInitialised() const;

	void Render(GameContext& context, World::World& world, float frameSeconds);
	// Drops every chunk's GPU buffers (the world was rebuilt or replaced).
	void ResetChunks();

private:
	struct RoleTexture
	{
		// Keeps the texture loaded in the resource manager; `handle` is its GPU texture.
		Resource::ResourcePtr<Resource::TextureResource> resource;
		bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
		uint32_t samplerFlags      = 0;
		int width       = 1;  // whole texture
		int height      = 1;
		int columns     = 1;
		int rows        = 1;
		bool placeholder = true;
	};

	struct ChunkGpu
	{
		double start   = 0.0;
		double originX = 0.0;
		double originZ = 0.0;
		bgfx::VertexBufferHandle terrainVertices = BGFX_INVALID_HANDLE;
		bgfx::IndexBufferHandle terrainIndices   = BGFX_INVALID_HANDLE;
		bgfx::VertexBufferHandle roadVertices    = BGFX_INVALID_HANDLE;
		bgfx::IndexBufferHandle roadIndices      = BGFX_INVALID_HANDLE;
		bgfx::VertexBufferHandle waterVertices   = BGFX_INVALID_HANDLE;
		bgfx::IndexBufferHandle waterIndices     = BGFX_INVALID_HANDLE;
		std::vector<bgfx::VertexBufferHandle> props;  // per prop type: static instance data
		std::vector<uint32_t> propCounts;
	};

	void LoadTexture(GameContext& context, const World::AssetRole& role, const World::ResolvedAsset& resolved);
	// Registers a generated image with the resource manager under `name` and makes it the role texture.
	bool UseImage(GameContext& context, const std::string& name, World::PlaceholderImage&& image, RoleTexture& texture);
	const RoleTexture& Texture(const std::string& role) const;
	void SyncChunks(World::World& world);
	void DestroyChunk(ChunkGpu& chunk);
	void EnsureFrameBuffer(uint16_t width, uint16_t height);
	void UpdateHeightTexture(const World::World& world);
	void SetSharedUniforms(const World::World& world);
	void DrawSky(const World::World& world);
	void DrawScene(World::World& world, entt::registry& registry);
	void DrawProps(const World::World& world, const ChunkGpu& chunk);
	void DrawActors(const World::World& world, entt::registry& registry);
	void DrawBanners(const World::World& world, entt::registry& registry);
	void DrawWeather(const World::World& world);
	void DrawRider(const World::World& world, entt::registry& registry);
	void DrawPost(const World::World& world, bgfx::FrameBufferHandle target, uint16_t width, uint16_t height);
	void DrawDebugOverlay(World::World& world, float frameSeconds);
	void SubmitSprite(bgfx::ViewId view, const RoleTexture& texture, const float corners[8], float u0, float v0,
	                  float u1, float v1, uint32_t abgr, uint64_t extraState = 0, uint32_t samplerFlags = UINT32_MAX);
	void SubmitBillboard(const World::World& world, const std::string& role, double distance, double lateral,
	                     double heightMeters, double angle, bool aligned, int column, int row, uint32_t tint,
	                     double towardsCamera, double baseHeight = 0.0);

	bool m_initialised = false;
	World::SpriteDescriptor m_descriptor;
	std::unordered_map<std::string, RoleTexture> m_textures;
	RoleTexture m_missing;
	std::unordered_map<int64_t, ChunkGpu> m_chunks;
	size_t m_propTypeCount = 0;

	bgfx::VertexLayout m_worldLayout;
	bgfx::VertexLayout m_instanceLayout;
	bgfx::VertexLayout m_screenLayout;
	bgfx::VertexLayout m_spriteLayout;
	bgfx::VertexBufferHandle m_propQuad   = BGFX_INVALID_HANDLE;
	bgfx::IndexBufferHandle m_quadIndices = BGFX_INVALID_HANDLE;

	bgfx::TextureHandle m_heightTexture = BGFX_INVALID_HANDLE;
	double m_heightBase                 = 0.0;
	std::vector<float> m_heightScratch;
	bool m_vertexTexturesSupported = true;

	bgfx::FrameBufferHandle m_frameBuffer = BGFX_INVALID_HANDLE;
	bgfx::TextureHandle m_sceneColor      = BGFX_INVALID_HANDLE;
	uint16_t m_frameWidth                 = 0;
	uint16_t m_frameHeight                = 0;

	// Per-frame camera, in the render frame (camera x/z at the origin).
	World::Camera m_camera;
	int m_powerZone = 0;
	double m_cameraX = 0.0;
	double m_cameraZ = 0.0;
	float m_eye[3]       = {0.0f, 0.0f, 0.0f};
	float m_right[3]     = {1.0f, 0.0f, 0.0f};
	float m_view[16]     = {};
	float m_projection[16] = {};
	float m_rotationViewProjection[16] = {};
	float m_sunDirection[3] = {0.0f, 1.0f, 0.0f};
	float m_sunStrength     = 1.0f;
	bool m_debugTextEnabled = false;
	// Where the rider stands on screen (low-res pixels) and how many pixels a metre is there - projected from the
	// rider's real position on the road, so the sprite sits on the road at the right size.
	float m_riderAnchorX     = 0.0f;
	float m_riderAnchorY     = 0.0f;
	float m_riderPixelHeight = 0.0f;
	// Per-frame counters for the debug overlay.
	uint32_t m_propDraws     = 0;
	uint32_t m_propInstances = 0;

	std::unordered_map<std::string, bgfx::UniformHandle> m_uniforms;
	bgfx::UniformHandle Uniform(const char* name, bgfx::UniformType::Enum type = bgfx::UniformType::Vec4);
	void SetVec4(const char* name, float x, float y, float z, float w);
};
}  // namespace Renderer
}  // namespace Struktur
