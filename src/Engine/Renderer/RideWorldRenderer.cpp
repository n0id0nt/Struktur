#include "RideWorldRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/EmbeddedShaders.h"
#include "Engine/Renderer/GraphicsDevice.h"
#include "Engine/Renderer/QuadVertex.h"
#include "Engine/Ride/Zones.h"
#include "Engine/World/Noise.h"
#include "Engine/World/Placeholder.h"

namespace Struktur::Renderer
{
static constexpr float PI                   = 3.14159265358979323846f;
static constexpr int HEIGHT_TEXTURE_WIDTH   = 2048;
static constexpr double HEIGHT_TEXTURE_BEHIND = 64.0;  // metres of road behind the rider kept in the height texture
static constexpr double STRIPE_PERIOD       = 1800.0;  // multiple of every road pattern period (dash 9 m, rumble 1.6 m)
static constexpr double TEXTURE_FOLD        = 1024.0;  // world texture coordinates are taken modulo this (precision)
static constexpr float NEAR_PLANE           = 0.3f;
static constexpr int MAX_CLOUDS             = 10;
static constexpr int MAX_WEATHER_PARTICLES  = 2000;
static constexpr float WEATHER_BOX          = 32.0f;  // metres around the camera
static constexpr float WEATHER_HEIGHT       = 16.0f;
static constexpr double WEATHER_NEAR_CLIP   = 2.5;  // particles closer than this to the eye would fill the screen
static constexpr uint64_t OPAQUE_STATE =
    BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS;
static constexpr uint32_t POINT_CLAMP = BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP;

struct ScreenVertex
{
	float x, y, z;
	float u, v, w;
};

RideWorldRenderer::~RideWorldRenderer()
{
	Shutdown();
}

bool RideWorldRenderer::IsInitialised() const
{
	return m_initialised;
}

bgfx::UniformHandle RideWorldRenderer::Uniform(const char* name, bgfx::UniformType::Enum type)
{
	const auto found = m_uniforms.find(name);
	if (found != m_uniforms.end())
	{
		return found->second;
	}
	const bgfx::UniformHandle handle = bgfx::createUniform(name, type);
	m_uniforms[name]                 = handle;
	return handle;
}

void RideWorldRenderer::SetVec4(const char* name, float x, float y, float z, float w)
{
	const float value[4] = {x, y, z, w};
	bgfx::setUniform(Uniform(name), value);
}

void RideWorldRenderer::Initialise(const World::AssetManifest& manifest, const World::SpriteDescriptor& descriptor)
{
	if (m_initialised)
	{
		return;
	}
	m_descriptor = descriptor;

	m_worldLayout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float)
	    .end();
	ASSERT_MSG(m_worldLayout.getStride() == sizeof(World::WorldVertex), "WorldVertex layout mismatch");
	// Instance data: two vec4 per instance (the attributes only give the stride; shaders read i_data0/i_data1).
	m_instanceLayout.begin()
	    .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord6, 4, bgfx::AttribType::Float)
	    .end();
	ASSERT_MSG(m_instanceLayout.getStride() == sizeof(World::PropInstance), "PropInstance layout mismatch");
	m_screenLayout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float)
	    .end();
	m_spriteLayout = BuildQuadVertexLayout();

	// Prop quad: bottom-centre origin, 1 x 1, UV top-left (0, 0).
	static const ScreenVertex propQuad[4] = {{-0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
	                                         {0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
	                                         {0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f},
	                                         {-0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f}};
	static const uint16_t quadIndices[6] = {0, 1, 2, 0, 2, 3};
	m_propQuad    = bgfx::createVertexBuffer(bgfx::makeRef(propQuad, sizeof(propQuad)), m_screenLayout);
	m_quadIndices = bgfx::createIndexBuffer(bgfx::makeRef(quadIndices, sizeof(quadIndices)));

	const bgfx::Caps* caps    = bgfx::getCaps();
	m_vertexTexturesSupported = (caps->formats[bgfx::TextureFormat::R32F] & BGFX_CAPS_FORMAT_TEXTURE_VERTEX) != 0;
	if (!m_vertexTexturesSupported)
	{
		DEBUG_WARNING("Ride world: this GPU can't sample R32F textures in vertex shaders - the road is drawn flat");
	}
	m_heightTexture = bgfx::createTexture2D(HEIGHT_TEXTURE_WIDTH, 1, false, 1, bgfx::TextureFormat::R32F, POINT_CLAMP);
	m_heightScratch.assign(HEIGHT_TEXTURE_WIDTH, 0.0f);

	// A 1x1 white texture for anything that fails to load at all.
	static const uint32_t white = 0xFFFFFFFFu;
	m_missing.handle = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, POINT_CLAMP, bgfx::copy(&white, 4));

	const std::vector<World::ResolvedAsset> resolved = World::ResolveAssets(manifest, FileSystem::Exists);
	for (const World::ResolvedAsset& asset : resolved)
	{
		LoadTexture(*asset.role, asset);
	}
	m_initialised = true;
}

void RideWorldRenderer::LoadTexture(const World::AssetRole& role, const World::ResolvedAsset& resolved)
{
	RoleTexture texture;
	texture.columns = std::max(1, role.frames);
	texture.rows    = std::max(1, role.rows);
	const uint32_t flags = BGFX_SAMPLER_POINT | (role.tileable ? 0u : static_cast<uint32_t>(BGFX_SAMPLER_UVW_CLAMP));
	if (resolved.found)
	{
		const FileResult<std::vector<uint8_t>> bytes = FileSystem::ReadBytes(resolved.path);
		if (bytes.success)
		{
			int width    = 0;
			int height   = 0;
			int channels = 0;
			stbi_uc* pixels =
			    stbi_load_from_memory(bytes.value.data(), static_cast<int>(bytes.value.size()), &width, &height, &channels, 4);
			if (pixels != nullptr && width > 0 && height > 0 && width <= 8192 && height <= 8192)
			{
				if (!role.regions.empty())
				{
					// Frames cut out of a larger sheet (tileset trees, a character atlas): build a strip of them.
					const World::PlaceholderImage strip = World::ComposeRegions(pixels, width, height, role);
					texture.handle = bgfx::createTexture2D(static_cast<uint16_t>(strip.width), static_cast<uint16_t>(strip.height),
					                                       false, 1, bgfx::TextureFormat::RGBA8, flags,
					                                       bgfx::copy(strip.pixels.data(), static_cast<uint32_t>(strip.pixels.size())));
					texture.width  = strip.width;
					texture.height = strip.height;
				}
				else
				{
					texture.handle = bgfx::createTexture2D(static_cast<uint16_t>(width), static_cast<uint16_t>(height), false, 1,
					                                       bgfx::TextureFormat::RGBA8, flags,
					                                       bgfx::copy(pixels, static_cast<uint32_t>(width * height * 4)));
					texture.width  = width;
					texture.height = height;
				}
				texture.placeholder = false;
			}
			if (pixels != nullptr)
			{
				stbi_image_free(pixels);
			}
		}
		if (!bgfx::isValid(texture.handle))
		{
			DEBUG_WARNING("Ride world: couldn't load %s for role '%s' - using %s", resolved.path.c_str(), role.role.c_str(),
			              resolved.placeholderName.c_str());
		}
	}
	if (!bgfx::isValid(texture.handle))
	{
		const World::PlaceholderImage image = World::GeneratePlaceholder(role, 1);
		texture.handle = bgfx::createTexture2D(static_cast<uint16_t>(image.width), static_cast<uint16_t>(image.height), false, 1,
		                                       bgfx::TextureFormat::RGBA8, flags,
		                                       bgfx::copy(image.pixels.data(), static_cast<uint32_t>(image.pixels.size())));
		texture.width       = image.width;
		texture.height      = image.height;
		texture.placeholder = true;
	}
	m_textures[role.role] = texture;
}

const RideWorldRenderer::RoleTexture& RideWorldRenderer::Texture(const std::string& role) const
{
	const auto found = m_textures.find(role);
	return found != m_textures.end() ? found->second : m_missing;
}

void RideWorldRenderer::DestroyChunk(ChunkGpu& chunk)
{
	const bgfx::VertexBufferHandle vertices[3] = {chunk.terrainVertices, chunk.roadVertices, chunk.waterVertices};
	const bgfx::IndexBufferHandle indices[3]   = {chunk.terrainIndices, chunk.roadIndices, chunk.waterIndices};
	for (const bgfx::VertexBufferHandle& handle : vertices)
	{
		if (bgfx::isValid(handle))
		{
			bgfx::destroy(handle);
		}
	}
	for (const bgfx::IndexBufferHandle& handle : indices)
	{
		if (bgfx::isValid(handle))
		{
			bgfx::destroy(handle);
		}
	}
	for (const bgfx::VertexBufferHandle& handle : chunk.props)
	{
		if (bgfx::isValid(handle))
		{
			bgfx::destroy(handle);
		}
	}
	chunk = ChunkGpu();
}

void RideWorldRenderer::ResetChunks()
{
	for (auto& [index, chunk] : m_chunks)
	{
		DestroyChunk(chunk);
	}
	m_chunks.clear();
}

void RideWorldRenderer::Shutdown()
{
	if (!m_initialised)
	{
		return;
	}
	ResetChunks();
	for (auto& [role, texture] : m_textures)
	{
		if (bgfx::isValid(texture.handle))
		{
			bgfx::destroy(texture.handle);
		}
	}
	m_textures.clear();
	const bgfx::TextureHandle textures[2] = {m_missing.handle, m_heightTexture};
	for (const bgfx::TextureHandle& handle : textures)
	{
		if (bgfx::isValid(handle))
		{
			bgfx::destroy(handle);
		}
	}
	m_missing       = RoleTexture();
	m_heightTexture = BGFX_INVALID_HANDLE;
	if (bgfx::isValid(m_frameBuffer))
	{
		bgfx::destroy(m_frameBuffer);  // also destroys its attachments
	}
	m_frameBuffer = BGFX_INVALID_HANDLE;
	m_sceneColor  = BGFX_INVALID_HANDLE;
	m_frameWidth  = 0;
	m_frameHeight = 0;
	if (bgfx::isValid(m_propQuad))
	{
		bgfx::destroy(m_propQuad);
	}
	if (bgfx::isValid(m_quadIndices))
	{
		bgfx::destroy(m_quadIndices);
	}
	m_propQuad    = BGFX_INVALID_HANDLE;
	m_quadIndices = BGFX_INVALID_HANDLE;
	for (auto& [name, handle] : m_uniforms)
	{
		bgfx::destroy(handle);
	}
	m_uniforms.clear();
	if (m_debugTextEnabled)
	{
		bgfx::setDebug(BGFX_DEBUG_NONE);
		m_debugTextEnabled = false;
	}
	m_initialised = false;
}

void RideWorldRenderer::EnsureFrameBuffer(uint16_t width, uint16_t height)
{
	if (bgfx::isValid(m_frameBuffer) && width == m_frameWidth && height == m_frameHeight)
	{
		return;
	}
	if (bgfx::isValid(m_frameBuffer))
	{
		bgfx::destroy(m_frameBuffer);
	}
	m_sceneColor = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8,
	                                     BGFX_TEXTURE_RT | BGFX_SAMPLER_UVW_CLAMP);
	const bgfx::TextureHandle depth =
	    bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT_WRITE_ONLY);
	const bgfx::TextureHandle attachments[2] = {m_sceneColor, depth};
	m_frameBuffer                             = bgfx::createFrameBuffer(2, attachments, true);
	m_frameWidth                              = width;
	m_frameHeight                             = height;
}

void RideWorldRenderer::SyncChunks(World::World& world)
{
	World::ChunkStreamer& streamer = world.GetStreamer();
	for (int64_t index : streamer.TakeReleased())
	{
		const auto found = m_chunks.find(index);
		if (found != m_chunks.end())
		{
			DestroyChunk(found->second);
			m_chunks.erase(found);
		}
	}
	const size_t propTypes = world.GetPropTypes() ? world.GetPropTypes()->roles.size() : 0;
	if (propTypes != m_propTypeCount)
	{
		m_propTypeCount = propTypes;
	}
	for (const World::ChunkMesh* mesh : streamer.TakeNewlyReady())
	{
		const auto existing = m_chunks.find(mesh->chunkIndex);
		if (existing != m_chunks.end())
		{
			DestroyChunk(existing->second);
		}
		ChunkGpu chunk;
		chunk.start   = mesh->start;
		chunk.originX = mesh->originX;
		chunk.originZ = mesh->originZ;
		if (!mesh->terrain.empty())
		{
			chunk.terrainVertices = bgfx::createVertexBuffer(
			    bgfx::copy(mesh->terrain.data(), static_cast<uint32_t>(mesh->terrain.size() * sizeof(World::WorldVertex))),
			    m_worldLayout);
			chunk.terrainIndices = bgfx::createIndexBuffer(
			    bgfx::copy(mesh->terrainIndices.data(), static_cast<uint32_t>(mesh->terrainIndices.size() * sizeof(uint16_t))));
		}
		if (!mesh->road.empty())
		{
			chunk.roadVertices = bgfx::createVertexBuffer(
			    bgfx::copy(mesh->road.data(), static_cast<uint32_t>(mesh->road.size() * sizeof(World::WorldVertex))),
			    m_worldLayout);
			chunk.roadIndices = bgfx::createIndexBuffer(
			    bgfx::copy(mesh->roadIndices.data(), static_cast<uint32_t>(mesh->roadIndices.size() * sizeof(uint16_t))));
		}
		if (!mesh->water.empty())
		{
			chunk.waterVertices = bgfx::createVertexBuffer(
			    bgfx::copy(mesh->water.data(), static_cast<uint32_t>(mesh->water.size() * sizeof(World::WorldVertex))),
			    m_worldLayout);
			chunk.waterIndices = bgfx::createIndexBuffer(
			    bgfx::copy(mesh->waterIndices.data(), static_cast<uint32_t>(mesh->waterIndices.size() * sizeof(uint16_t))));
		}
		chunk.props.assign(mesh->props.size(), BGFX_INVALID_HANDLE);
		chunk.propCounts.assign(mesh->props.size(), 0);
		for (size_t type = 0; type < mesh->props.size(); ++type)
		{
			const std::vector<World::PropInstance>& instances = mesh->props[type];
			if (instances.empty())
			{
				continue;
			}
			chunk.props[type] = bgfx::createVertexBuffer(
			    bgfx::copy(instances.data(), static_cast<uint32_t>(instances.size() * sizeof(World::PropInstance))),
			    m_instanceLayout);
			chunk.propCounts[type] = static_cast<uint32_t>(instances.size());
		}
		m_chunks[mesh->chunkIndex] = std::move(chunk);
	}
}

void RideWorldRenderer::UpdateHeightTexture(const World::World& world)
{
	const double spacing = world.GetProfile().GetLimits().spacingMeters;
	m_heightBase         = std::floor((world.GetDistance() - HEIGHT_TEXTURE_BEHIND) / spacing) * spacing;
	world.GetProfile().FillRelativeHeights(m_heightBase, world.GetHeightOrigin(), HEIGHT_TEXTURE_WIDTH, m_heightScratch);
	bgfx::updateTexture2D(m_heightTexture, 0, 0, 0, 0, HEIGHT_TEXTURE_WIDTH, 1,
	                      bgfx::copy(m_heightScratch.data(), HEIGHT_TEXTURE_WIDTH * sizeof(float)));
}

static void ColorUniformArgs(const World::Color& color, float out[3])
{
	out[0] = color.red;
	out[1] = color.green;
	out[2] = color.blue;
}

void RideWorldRenderer::SetSharedUniforms(const World::World& world)
{
	const World::BiomeLook& look       = world.GetLook();
	const World::WorldSettings& settings = world.GetSettings();
	const double spacing                   = world.GetProfile().GetLimits().spacingMeters;
	SetVec4("u_heightParams", static_cast<float>(spacing), static_cast<float>(HEIGHT_TEXTURE_WIDTH),
	        m_vertexTexturesSupported ? 1.0f : 0.0f, static_cast<float>(std::fmod(world.GetTimeSeconds(), 3600.0)));
	SetVec4("u_cameraPos", m_eye[0], m_eye[1], m_eye[2], 0.0f);
	SetVec4("u_fogColor", look.fogColor.red, look.fogColor.green, look.fogColor.blue, static_cast<float>(look.fogDensity));
	SetVec4("u_fogParams", static_cast<float>(look.fogStartMeters), settings.bandedFog ? static_cast<float>(settings.fogBands) : 0.0f,
	        1.0f, 0.0f);
	SetVec4("u_sunDir", m_sunDirection[0], m_sunDirection[1], m_sunDirection[2], m_sunStrength);
	SetVec4("u_sunColor", look.sunColor.red * 0.75f, look.sunColor.green * 0.75f, look.sunColor.blue * 0.75f, 1.0f);
	SetVec4("u_ambient", look.ambient.red, look.ambient.green, look.ambient.blue, 1.0f);
	SetVec4("u_cameraRight", m_right[0], m_right[1], m_right[2], 0.0f);

	const World::BiomeBlend blend               = world.GetBlend();
	const std::vector<World::BiomeDefinition>& biomes = world.GetBiomes();
	float line[3]     = {0.9f, 0.9f, 0.9f};
	float shoulder[3] = {0.4f, 0.4f, 0.4f};
	if (!biomes.empty())
	{
		const World::BiomeDefinition& from = biomes[static_cast<size_t>(blend.from)];
		const World::BiomeDefinition& to   = biomes[static_cast<size_t>(blend.to)];
		ColorUniformArgs(World::Color::Lerp(from.roadLine, to.roadLine, blend.weight), line);
		ColorUniformArgs(World::Color::Lerp(from.roadShoulder, to.roadShoulder, blend.weight), shoulder);
	}
	SetVec4("u_road", static_cast<float>(settings.roadHalfWidthMeters), static_cast<float>(settings.shoulderMeters), 0.15f, 0.45f);
	SetVec4("u_roadLine", line[0], line[1], line[2], 1.0f);
	SetVec4("u_roadShoulder", shoulder[0], shoulder[1], shoulder[2], 1.0f);
	const Ride::ZoneColor zone = Ride::PowerZoneColor(std::clamp(world.GetPowerZone(), 0, Ride::POWER_ZONE_COUNT));
	SetVec4("u_zoneColor", zone.red / 255.0f, zone.green / 255.0f, zone.blue / 255.0f, 1.0f);
}

void RideWorldRenderer::SubmitSprite(bgfx::ViewId view, const RoleTexture& texture, const float corners[8], float u0,
                                     float v0, float u1, float v1, uint32_t abgr, uint64_t extraState,
                                     uint32_t samplerFlags)
{
	if (bgfx::getAvailTransientVertexBuffer(4, m_spriteLayout) < 4 || bgfx::getAvailTransientIndexBuffer(6) < 6)
	{
		return;
	}
	bgfx::TransientVertexBuffer vertices;
	bgfx::TransientIndexBuffer indices;
	bgfx::allocTransientBuffers(&vertices, m_spriteLayout, 4, &indices, 6);
	QuadVertex* quad = reinterpret_cast<QuadVertex*>(vertices.data);
	const float us[4] = {u0, u1, u1, u0};
	const float vs[4] = {v0, v0, v1, v1};
	for (int corner = 0; corner < 4; ++corner)
	{
		quad[corner].x    = corners[corner * 2];
		quad[corner].y    = corners[corner * 2 + 1];
		quad[corner].u    = us[corner];
		quad[corner].v    = vs[corner];
		quad[corner].abgr = abgr;
	}
	uint16_t* index      = reinterpret_cast<uint16_t*>(indices.data);
	const uint16_t order[6] = {0, 1, 2, 0, 2, 3};
	std::memcpy(index, order, sizeof(order));
	bgfx::setVertexBuffer(0, &vertices);
	bgfx::setIndexBuffer(&indices);
	bgfx::setTexture(0, Uniform("s_texColor", bgfx::UniformType::Sampler), texture.handle, samplerFlags);
	const uint64_t blend = extraState != 0 ? extraState : BGFX_STATE_BLEND_ALPHA;
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | blend);
	bgfx::submit(view, GetEmbeddedProgram("sprite"));
}

static void ScreenQuad(float width, float height, float u0, float v0, float u1, float v1, ScreenVertex out[4])
{
	out[0] = ScreenVertex{0.0f, 0.0f, 0.0f, u0, v0, 0.0f};
	out[1] = ScreenVertex{width, 0.0f, 0.0f, u1, v0, 0.0f};
	out[2] = ScreenVertex{width, height, 0.0f, u1, v1, 0.0f};
	out[3] = ScreenVertex{0.0f, height, 0.0f, u0, v1, 0.0f};
}

static bool SubmitScreenQuad(const bgfx::VertexLayout& layout, const ScreenVertex quad[4])
{
	if (bgfx::getAvailTransientVertexBuffer(4, layout) < 4 || bgfx::getAvailTransientIndexBuffer(6) < 6)
	{
		return false;
	}
	bgfx::TransientVertexBuffer vertices;
	bgfx::TransientIndexBuffer indices;
	bgfx::allocTransientBuffers(&vertices, layout, 4, &indices, 6);
	std::memcpy(vertices.data, quad, sizeof(ScreenVertex) * 4);
	const uint16_t order[6] = {0, 1, 2, 0, 2, 3};
	std::memcpy(indices.data, order, sizeof(order));
	bgfx::setVertexBuffer(0, &vertices);
	bgfx::setIndexBuffer(&indices);
	return true;
}

// Direction towards a sky object from its height above the horizon and heading (relative to the route's mean
// direction, +x), y up.
static glm::vec3 SkyDirection(float elevationDegrees, float azimuthDegrees)
{
	const float elevation = elevationDegrees * PI / 180.0f;
	const float azimuth   = azimuthDegrees * PI / 180.0f;
	return glm::vec3(std::cos(elevation) * std::cos(azimuth), std::sin(elevation), std::cos(elevation) * std::sin(azimuth));
}

// Screen position (pixels, top-left origin) of a direction at infinity; false when it's behind the camera.
static bool ProjectToScreen(const glm::mat4& rotationViewProjection, const glm::vec3& direction, float width, float height,
                            glm::vec2& screen)
{
	const glm::vec4 clip = rotationViewProjection * glm::vec4(direction, 1.0f);
	if (clip.w <= 0.0f)
	{
		return false;
	}
	screen = glm::vec2((clip.x / clip.w * 0.5f + 0.5f) * width, (1.0f - (clip.y / clip.w * 0.5f + 0.5f)) * height);
	return true;
}

void RideWorldRenderer::DrawSky(const World::World& world)
{
	const bgfx::ViewId view          = GraphicsDevice::RideSkyViewId;
	const World::BiomeLook& look = world.GetLook();
	const float width                = m_frameWidth;
	const float height               = m_frameHeight;
	const glm::mat4 ortho            = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
	const glm::mat4 identity(1.0f);
	bgfx::setViewName(view, "Ride sky");
	bgfx::setViewFrameBuffer(view, m_frameBuffer);
	bgfx::setViewRect(view, 0, 0, m_frameWidth, m_frameHeight);
	// bgfx clear colours are 0xRRGGBBAA.
	const uint32_t clear = (static_cast<uint32_t>(look.fogColor.red * 255.0f) << 24) |
	                       (static_cast<uint32_t>(look.fogColor.green * 255.0f) << 16) |
	                       (static_cast<uint32_t>(look.fogColor.blue * 255.0f) << 8) | 0xFFu;
	bgfx::setViewClear(view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clear, 1.0f, 0);
	bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
	bgfx::setViewTransform(view, glm::value_ptr(identity), glm::value_ptr(ortho));

	const glm::mat4 inverse = glm::inverse(glm::make_mat4(m_rotationViewProjection));
	bgfx::setUniform(Uniform("u_skyInvViewProj", bgfx::UniformType::Mat4), glm::value_ptr(inverse));
	SetVec4("u_skyZenith", look.skyZenith.red, look.skyZenith.green, look.skyZenith.blue, 1.0f);
	SetVec4("u_skyHorizon", look.skyHorizon.red, look.skyHorizon.green, look.skyHorizon.blue, 1.0f);
	SetVec4("u_skyGround", look.skyGround.red, look.skyGround.green, look.skyGround.blue, 1.0f);
	const bool day = m_sunDirection[1] > 0.0f && look.starIntensity < 0.5;
	SetVec4("u_skySun", m_sunDirection[0], m_sunDirection[1], m_sunDirection[2], day ? 1.0f : 0.15f);
	SetVec4("u_skySunColor", look.sunColor.red, look.sunColor.green, look.sunColor.blue, 1.0f);
	// Mountains: band height as a fraction of the vertical field of view; slow parallax with distance.
	const float fovRadians = static_cast<float>(world.GetCamera().fieldOfViewDegrees) * PI / 180.0f;
	const double distance  = world.GetDistance();
	SetVec4("u_skyParams", static_cast<float>(look.starIntensity), static_cast<float>(look.mountainHeight) * fovRadians,
	        static_cast<float>(std::fmod(distance * 0.000012, 1.0)), static_cast<float>(std::fmod(distance * 0.00004, 1.0)));
	const World::Color far  = World::Color::Lerp(look.mountainFar, look.fogColor, 0.35f);
	const World::Color near = World::Color::Lerp(look.mountainNear, look.fogColor, 0.15f);
	SetVec4("u_skyMountainFar", far.red, far.green, far.blue, 1.0f);
	SetVec4("u_skyMountainNear", near.red, near.green, near.blue, 1.0f);

	ScreenVertex quad[4];
	ScreenQuad(width, height, -1.0f, 1.0f, 1.0f, -1.0f, quad);
	if (SubmitScreenQuad(m_screenLayout, quad))
	{
		bgfx::setTexture(0, Uniform("s_stars", bgfx::UniformType::Sampler), Texture("sky_stars").handle);
		bgfx::setTexture(1, Uniform("s_mountainsFar", bgfx::UniformType::Sampler), Texture("mountains_far").handle);
		bgfx::setTexture(2, Uniform("s_mountainsNear", bgfx::UniformType::Sampler), Texture("mountains_near").handle);
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
		bgfx::submit(view, GetEmbeddedProgram("rideSky"));
	}

	// Sun, moon and clouds: sprites at their projected screen positions (they're at infinity: rotation only).
	const glm::mat4 rotationViewProjection = glm::make_mat4(m_rotationViewProjection);
	glm::vec2 screen;
	const glm::vec3 sun = glm::vec3(m_sunDirection[0], m_sunDirection[1], m_sunDirection[2]);
	if (day && ProjectToScreen(rotationViewProjection, sun, width, height, screen))
	{
		const float size     = height * 0.11f * static_cast<float>(look.sunSize);
		const float corners[8] = {screen.x - size, screen.y - size, screen.x + size, screen.y - size,
		                          screen.x + size, screen.y + size, screen.x - size, screen.y + size};
		SubmitSprite(view, Texture("sky_sun"), corners, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFFu, BGFX_STATE_BLEND_ADD);
	}
	if (look.moonIntensity > 0.05)
	{
		const glm::vec3 moon = SkyDirection(32.0f, static_cast<float>(look.sunAzimuthDeg) + 150.0f);
		if (ProjectToScreen(rotationViewProjection, moon, width, height, screen))
		{
			const float size     = height * 0.07f;
			const float corners[8] = {screen.x - size, screen.y - size, screen.x + size, screen.y - size,
			                          screen.x + size, screen.y + size, screen.x - size, screen.y + size};
			const uint8_t alpha  = static_cast<uint8_t>(std::clamp(look.moonIntensity, 0.0, 1.0) * 255.0);
			SubmitSprite(view, Texture("sky_moon"), corners, 0.0f, 0.0f, 1.0f, 1.0f, (static_cast<uint32_t>(alpha) << 24) | 0xFFFFFFu);
		}
	}
	const int clouds = std::clamp(static_cast<int>(std::lround(look.cloudAmount * MAX_CLOUDS)), 0, MAX_CLOUDS);
	const RoleTexture& cloud = Texture("sky_clouds");
	const World::Color tint = World::Color::Lerp(look.skyHorizon, World::Color{1.0f, 1.0f, 1.0f},
	                                                     day ? 0.7f : 0.25f);
	const float time = static_cast<float>(world.GetTimeSeconds());
	for (int index = 0; index < clouds; ++index)
	{
		const float azimuth   = static_cast<float>(World::HashUnit(7, index) * 360.0) + time * 0.15f;
		const float elevation = 3.0f + static_cast<float>(World::HashUnit(8, index) * 14.0);
		if (!ProjectToScreen(rotationViewProjection, SkyDirection(elevation, azimuth), width, height, screen))
		{
			continue;
		}
		const float cloudWidth  = height * (0.22f + static_cast<float>(World::HashUnit(9, index)) * 0.25f);
		const float cloudHeight = cloudWidth * static_cast<float>(cloud.height) / static_cast<float>(cloud.width);
		const float corners[8]  = {screen.x - cloudWidth * 0.5f, screen.y - cloudHeight * 0.5f,
		                           screen.x + cloudWidth * 0.5f, screen.y - cloudHeight * 0.5f,
		                           screen.x + cloudWidth * 0.5f, screen.y + cloudHeight * 0.5f,
		                           screen.x - cloudWidth * 0.5f, screen.y + cloudHeight * 0.5f};
		SubmitSprite(view, cloud, corners, 0.0f, 0.0f, 1.0f, 1.0f, tint.ToAbgr(0.85f));
	}
}

void RideWorldRenderer::DrawProps(const World::World& world, const ChunkGpu& chunk)
{
	const std::shared_ptr<const World::PropTypeTable> table = world.GetPropTypes();
	if (!table)
	{
		return;
	}
	const double ahead = chunk.start - world.GetDistance();
	for (size_t type = 0; type < chunk.props.size() && type < table->roles.size(); ++type)
	{
		const uint32_t count = chunk.propCounts[type];
		if (count == 0 || ahead > table->maxDistance[type])
		{
			continue;
		}
		const RoleTexture& texture = Texture(table->roles[type]);
		const float frameWidth     = static_cast<float>(texture.width) / texture.columns;
		const float frameHeight    = static_cast<float>(texture.height) / texture.rows;
		SetVec4("u_propSheet", static_cast<float>(texture.columns), static_cast<float>(texture.rows), 0.0f, frameWidth / frameHeight);
		SetVec4("u_propMode", table->alignToRoad[type] ? 1.0f : 0.0f, 0.3f, 0.0f, 0.5f);
		SetVec4("u_propTint", 1.0f, 1.0f, 1.0f, 1.0f);
		const glm::mat4 transform =
		    glm::translate(glm::mat4(1.0f), glm::vec3(static_cast<float>(chunk.originX - m_cameraX), 0.0f,
		                                              static_cast<float>(chunk.originZ - m_cameraZ)));
		bgfx::setTransform(glm::value_ptr(transform));
		bgfx::setVertexBuffer(0, m_propQuad);
		bgfx::setIndexBuffer(m_quadIndices);
		// One instanced draw per prop type per chunk, from the chunk's static instance buffer.
		bgfx::setInstanceDataBuffer(chunk.props[type], 0, count);
		bgfx::setTexture(0, Uniform("s_height", bgfx::UniformType::Sampler), m_heightTexture);
		bgfx::setTexture(1, Uniform("s_texColor", bgfx::UniformType::Sampler), texture.handle);
		bgfx::setState(OPAQUE_STATE);
		bgfx::submit(GraphicsDevice::RideSceneViewId, GetEmbeddedProgram("rideProp"));
		++m_propDraws;
		m_propInstances += count;
	}
}

void RideWorldRenderer::SubmitBillboard(const World::World& world, const std::string& role, double distance,
                                        double lateral, double heightMeters, double angle, bool aligned, int column,
                                        int row, uint32_t tint, double towardsCamera, double baseHeight)
{
	if (bgfx::getAvailInstanceDataBuffer(1, sizeof(World::PropInstance)) < 1)
	{
		return;
	}
	const World::RouteSample sample = world.GetRoute().SampleAt(distance);
	float x = static_cast<float>(sample.x + sample.LeftX() * lateral - m_cameraX);
	float z = static_cast<float>(sample.z + sample.LeftZ() * lateral - m_cameraZ);
	// Nudge layered sprites towards the camera so their layers don't z-fight.
	const float toEyeX  = m_eye[0] - x;
	const float toEyeZ  = m_eye[2] - z;
	const float length  = std::max(0.001f, std::sqrt(toEyeX * toEyeX + toEyeZ * toEyeZ));
	x += toEyeX / length * static_cast<float>(towardsCamera);
	z += toEyeZ / length * static_cast<float>(towardsCamera);

	bgfx::InstanceDataBuffer instances;
	bgfx::allocInstanceDataBuffer(&instances, 1, sizeof(World::PropInstance));
	World::PropInstance* instance = reinterpret_cast<World::PropInstance*>(instances.data);
	const RoleTexture& texture        = Texture(role);
	instance->x            = x;
	instance->y            = static_cast<float>(baseHeight);
	instance->z            = z;
	instance->distance     = 0.0f;
	instance->heightMeters = static_cast<float>(heightMeters);
	instance->angle        = static_cast<float>(angle);
	instance->variant      = (static_cast<float>(column) + 0.5f) / static_cast<float>(texture.columns);
	instance->tint         = 0.5f;
	const float frameWidth  = static_cast<float>(texture.width) / texture.columns;
	const float frameHeight = static_cast<float>(texture.height) / texture.rows;
	SetVec4("u_chunk", static_cast<float>(distance - m_heightBase), 0.0f, 0.0f, 0.0f);
	SetVec4("u_propSheet", static_cast<float>(texture.columns), static_cast<float>(texture.rows), static_cast<float>(row),
	        frameWidth / frameHeight);
	SetVec4("u_propMode", aligned ? 1.0f : 0.0f, 0.0f, 0.0f, 0.5f);
	SetVec4("u_propTint", (tint & 0xFF) / 255.0f, ((tint >> 8) & 0xFF) / 255.0f, ((tint >> 16) & 0xFF) / 255.0f, 1.0f);
	const glm::mat4 identity(1.0f);
	bgfx::setTransform(glm::value_ptr(identity));
	bgfx::setVertexBuffer(0, m_propQuad);
	bgfx::setIndexBuffer(m_quadIndices);
	bgfx::setInstanceDataBuffer(&instances);
	bgfx::setTexture(0, Uniform("s_height", bgfx::UniformType::Sampler), m_heightTexture);
	bgfx::setTexture(1, Uniform("s_texColor", bgfx::UniformType::Sampler), texture.handle);
	bgfx::setState(OPAQUE_STATE);
	bgfx::submit(GraphicsDevice::RideSceneViewId, GetEmbeddedProgram("rideProp"));
}

// Which frame (column, row) of a crank layer to show for a pose.
static void CrankCell(const World::SpriteLayer& layer, const World::SpriteDescriptor& descriptor,
                      World::RiderPose pose, double phase, int& column, int& row)
{
	row    = layer.pedalRow;
	column = World::CrankFrame(phase, layer.framesPerRevolution > 0 ? layer.framesPerRevolution : descriptor.framesPerRevolution);
	if (pose == World::RiderPose::OUT_OF_SADDLE && layer.outOfSaddleRow >= 0)
	{
		row = layer.outOfSaddleRow;
	}
	else if (pose == World::RiderPose::COASTING)
	{
		row    = layer.stillRow >= 0 ? layer.stillRow : layer.pedalRow;
		column = layer.coastColumn;
	}
	else if (pose == World::RiderPose::IDLE)
	{
		row    = layer.stillRow >= 0 ? layer.stillRow : layer.pedalRow;
		column = layer.stillRow >= 0 ? layer.idleColumn : layer.coastColumn;
	}
}

void RideWorldRenderer::DrawActors(const World::World& world)
{
	const double height = m_descriptor.heightMeters;
	for (const World::WorldActor& actor : world.GetActors())
	{
		if (actor.distance < world.GetDistance() - 2.0 ||
		    actor.distance > world.GetDistance() + world.GetSettings().Resolved().drawDistanceMeters)
		{
			continue;
		}
		double offset = 0.0;
		for (const World::SpriteLayer& layer : m_descriptor.layers)
		{
			if (layer.motion == World::SpriteLayerMotion::WHEEL_SCROLL)
			{
				continue;  // the tread strip only makes sense on the close-up player sprite
			}
			int column = 0;
			int row    = 0;
			if (layer.motion == World::SpriteLayerMotion::CRANK)
			{
				CrankCell(layer, m_descriptor, World::RiderPose::PEDALLING, actor.crankPhase, column, row);
			}
			// A layer with its own rect (e.g. the character over the bike) is scaled and lifted to match the frame.
			double layerHeight = height;
			double baseHeight  = 0.0;
			if (layer.rectHeight > 0)
			{
				layerHeight = height * layer.rectHeight / m_descriptor.frameHeight;
				baseHeight  = height * (m_descriptor.anchorY - (layer.rectY + layer.rectHeight)) / m_descriptor.frameHeight;
			}
			const bool tinted = layer.name == "torso";
			SubmitBillboard(world, layer.role, actor.distance, actor.lateral, layerHeight, 0.0, false, column, row,
			                tinted ? actor.tint : 0xFFFFFFFFu, offset, baseHeight);
			offset += 0.02;
		}
	}
}

void RideWorldRenderer::DrawBanners(const World::World& world)
{
	for (double distance : world.GetBanners())
	{
		const World::RouteSample sample = world.GetRoute().SampleAt(distance);
		// Aligned quad across the road: its "right" runs along the road's left vector.
		const double angle = std::atan2(sample.LeftZ(), sample.LeftX());
		SubmitBillboard(world, "finish_banner", distance, 0.0, 4.2, angle, true, 0, 0, 0xFFFFFFFFu, 0.0);
	}
}

void RideWorldRenderer::DrawWeather(const World::World& world)
{
	const World::BiomeLook& look = world.GetLook();
	const bool snow                  = look.snow > look.rain;
	const double intensity           = snow ? look.snow : look.rain;
	if (intensity < 0.02)
	{
		return;
	}
	const int wanted = std::min(MAX_WEATHER_PARTICLES,
	                            static_cast<int>(world.GetSettings().Resolved().weatherParticles * intensity));
	const uint32_t count = bgfx::getAvailInstanceDataBuffer(static_cast<uint32_t>(wanted), sizeof(World::PropInstance));
	if (count == 0)
	{
		return;
	}
	bgfx::InstanceDataBuffer instances;
	bgfx::allocInstanceDataBuffer(&instances, count, sizeof(World::PropInstance));
	World::PropInstance* particle = reinterpret_cast<World::PropInstance*>(instances.data);
	const double time   = world.GetTimeSeconds();
	const double fall   = snow ? 1.3 : 11.0;
	const double drift  = snow ? 0.8 : 0.0;
	const double cameraY = world.GetCamera().y;
	for (uint32_t index = 0; index < count; ++index)
	{
		// Each particle lives at a hashed spot in a box that wraps around the camera as it moves and as time passes.
		const double baseX = World::HashUnit(31, index) * WEATHER_BOX;
		const double baseZ = World::HashUnit(32, index) * WEATHER_BOX;
		const double baseY = World::HashUnit(33, index) * WEATHER_HEIGHT;
		const double worldX = baseX + std::sin(time * 0.7 + index) * drift;
		const double worldZ = baseZ + std::cos(time * 0.6 + index * 1.3) * drift;
		const double wrappedX = std::fmod(std::fmod(worldX - m_cameraX, WEATHER_BOX) + WEATHER_BOX, WEATHER_BOX) - WEATHER_BOX * 0.5;
		const double wrappedZ = std::fmod(std::fmod(worldZ - m_cameraZ, WEATHER_BOX) + WEATHER_BOX, WEATHER_BOX) - WEATHER_BOX * 0.5;
		const double wrappedY = std::fmod(std::fmod(baseY - time * fall, WEATHER_HEIGHT) + WEATHER_HEIGHT, WEATHER_HEIGHT);
		const double worldY = cameraY - 4.0 + wrappedY;
		const double nearX  = wrappedX;
		const double nearY  = worldY - cameraY;
		const double nearZ  = wrappedZ;
		const bool tooClose = nearX * nearX + nearY * nearY + nearZ * nearZ < WEATHER_NEAR_CLIP * WEATHER_NEAR_CLIP;
		particle[index].x            = static_cast<float>(wrappedX);
		particle[index].y            = static_cast<float>(worldY);
		particle[index].z            = static_cast<float>(wrappedZ);
		particle[index].distance     = 0.0f;
		particle[index].heightMeters = tooClose ? 0.0f : (snow ? 0.07f : 0.45f);
		particle[index].angle        = 0.0f;
		particle[index].variant      = static_cast<float>(World::HashUnit(34, index));
		particle[index].tint         = 0.5f;
	}
	const RoleTexture& texture = Texture(snow ? "particle_snow" : "particle_rain");
	const float frameWidth     = static_cast<float>(texture.width) / texture.columns;
	const float frameHeight    = static_cast<float>(texture.height) / texture.rows;
	// Particles are already camera-relative and absolute in height: no road height.
	SetVec4("u_heightParams", 1.0f, static_cast<float>(HEIGHT_TEXTURE_WIDTH), 0.0f, 0.0f);
	SetVec4("u_chunk", 0.0f, 0.0f, 0.0f, 0.0f);
	SetVec4("u_propSheet", static_cast<float>(texture.columns), static_cast<float>(texture.rows), 0.0f, frameWidth / frameHeight);
	// Particle sheets are faint (soft alpha): keep anything visible.
	SetVec4("u_propMode", 0.0f, 0.0f, 0.0f, 0.05f);
	SetVec4("u_propTint", 1.0f, 1.0f, 1.0f, snow ? 1.0f : 0.0f);
	const glm::mat4 identity(1.0f);
	bgfx::setTransform(glm::value_ptr(identity));
	bgfx::setVertexBuffer(0, m_propQuad);
	bgfx::setIndexBuffer(m_quadIndices);
	bgfx::setInstanceDataBuffer(&instances);
	bgfx::setTexture(0, Uniform("s_height", bgfx::UniformType::Sampler), m_heightTexture);
	bgfx::setTexture(1, Uniform("s_texColor", bgfx::UniformType::Sampler), texture.handle);
	bgfx::setState(OPAQUE_STATE);
	bgfx::submit(GraphicsDevice::RideSceneViewId, GetEmbeddedProgram("rideProp"));
}

void RideWorldRenderer::DrawScene(World::World& world)
{
	const bgfx::ViewId view = GraphicsDevice::RideSceneViewId;
	bgfx::setViewName(view, "Ride scene");
	bgfx::setViewFrameBuffer(view, m_frameBuffer);
	bgfx::setViewRect(view, 0, 0, m_frameWidth, m_frameHeight);
	bgfx::setViewClear(view, BGFX_CLEAR_NONE);
	// Sequential: per-draw uniforms (u_chunk, u_propSheet...) are deltas in submission order, so draws mustn't be
	// reordered. The scene is opaque + alpha-tested, so order doesn't matter visually.
	bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
	bgfx::setViewTransform(view, m_view, m_projection);

	const double drawDistance = world.GetSettings().Resolved().drawDistanceMeters;
	const bgfx::UniformHandle height = Uniform("s_height", bgfx::UniformType::Sampler);
	for (const auto& [index, chunk] : m_chunks)
	{
		if (chunk.start > world.GetDistance() + drawDistance || chunk.start + 64.0 < world.GetDistance() - 70.0)
		{
			continue;
		}
		SetVec4("u_chunk", static_cast<float>(chunk.start - m_heightBase), static_cast<float>(std::fmod(chunk.start, STRIPE_PERIOD)),
		        static_cast<float>(std::fmod(chunk.originX, TEXTURE_FOLD)), static_cast<float>(std::fmod(chunk.originZ, TEXTURE_FOLD)));
		const glm::mat4 transform =
		    glm::translate(glm::mat4(1.0f), glm::vec3(static_cast<float>(chunk.originX - m_cameraX), 0.0f,
		                                              static_cast<float>(chunk.originZ - m_cameraZ)));
		const bgfx::VertexBufferHandle meshes[2]  = {chunk.terrainVertices, chunk.waterVertices};
		const bgfx::IndexBufferHandle indices[2] = {chunk.terrainIndices, chunk.waterIndices};
		for (int mesh = 0; mesh < 2; ++mesh)
		{
			if (!bgfx::isValid(meshes[mesh]))
			{
				continue;
			}
			bgfx::setTransform(glm::value_ptr(transform));
			bgfx::setVertexBuffer(0, meshes[mesh]);
			bgfx::setIndexBuffer(indices[mesh]);
			bgfx::setTexture(0, height, m_heightTexture);
			bgfx::setTexture(1, Uniform("s_grass", bgfx::UniformType::Sampler), Texture("terrain_grass").handle);
			bgfx::setTexture(2, Uniform("s_rock", bgfx::UniformType::Sampler), Texture("terrain_rock").handle);
			bgfx::setTexture(3, Uniform("s_snow", bgfx::UniformType::Sampler), Texture("terrain_snow").handle);
			bgfx::setTexture(4, Uniform("s_sand", bgfx::UniformType::Sampler), Texture("terrain_sand").handle);
			bgfx::setTexture(5, Uniform("s_water", bgfx::UniformType::Sampler), Texture("water").handle);
			bgfx::setState(OPAQUE_STATE);
			bgfx::submit(view, GetEmbeddedProgram("rideTerrain"));
		}
		if (bgfx::isValid(chunk.roadVertices))
		{
			bgfx::setTransform(glm::value_ptr(transform));
			bgfx::setVertexBuffer(0, chunk.roadVertices);
			bgfx::setIndexBuffer(chunk.roadIndices);
			bgfx::setTexture(0, height, m_heightTexture);
			bgfx::setTexture(1, Uniform("s_asphalt", bgfx::UniformType::Sampler), Texture("road_asphalt").handle);
			bgfx::setTexture(2, Uniform("s_markings", bgfx::UniformType::Sampler), Texture("road_markings").handle);
			bgfx::setState(OPAQUE_STATE);
			bgfx::submit(view, GetEmbeddedProgram("rideRoad"));
		}
		DrawProps(world, chunk);
	}
	DrawBanners(world);
	DrawActors(world);
	DrawWeather(world);
}

void RideWorldRenderer::DrawRider(const World::World& world)
{
	const bgfx::ViewId view = GraphicsDevice::RideOverlayViewId;
	const float width       = m_frameWidth;
	const float height      = m_frameHeight;
	const glm::mat4 ortho   = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
	const glm::mat4 identity(1.0f);
	bgfx::setViewName(view, "Ride rider");
	bgfx::setViewFrameBuffer(view, m_frameBuffer);
	bgfx::setViewRect(view, 0, 0, m_frameWidth, m_frameHeight);
	bgfx::setViewClear(view, BGFX_CLEAR_NONE);
	bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
	bgfx::setViewTransform(view, glm::value_ptr(identity), glm::value_ptr(ortho));

	const World::RiderAnimation& animation = world.GetRiderAnimation();
	const World::SpriteDescriptor& sprite  = m_descriptor;
	// Projected from the rider's place on the road; if that failed, the descriptor's fixed size at the bottom centre.
	float scale   = static_cast<float>(sprite.screenHeight) * height / static_cast<float>(sprite.frameHeight);
	float anchorX = width * 0.5f;
	float anchorY = height - 2.0f;
	if (m_riderPixelHeight > 1.0f)
	{
		// heightMeters is the frame's full height.
		scale   = m_riderPixelHeight / static_cast<float>(sprite.frameHeight);
		anchorX = m_riderAnchorX;
		anchorY = m_riderAnchorY;
	}
	int leanRow         = -1;
	float lean          = sprite.lean ? static_cast<float>(animation.GetLeanDegrees()) : 0.0f;
	if (lean > 8.0f && sprite.leanLeftRow >= 0)
	{
		leanRow = sprite.leanLeftRow;
		lean    = 0.0f;
	}
	else if (lean < -8.0f && sprite.leanRightRow >= 0)
	{
		leanRow = sprite.leanRightRow;
		lean    = 0.0f;
	}
	const float leanRadians = lean * PI / 180.0f;
	const float cosine      = std::cos(leanRadians);
	const float sine        = std::sin(leanRadians);

	// Shadow first, unrotated.
	{
		const float shadowWidth  = sprite.frameWidth * scale * 0.85f;
		const float shadowHeight = shadowWidth * 0.22f;
		const float corners[8]   = {anchorX - shadowWidth * 0.5f, anchorY - shadowHeight * 0.6f,
		                            anchorX + shadowWidth * 0.5f, anchorY - shadowHeight * 0.6f,
		                            anchorX + shadowWidth * 0.5f, anchorY + shadowHeight * 0.4f,
		                            anchorX - shadowWidth * 0.5f, anchorY + shadowHeight * 0.4f};
		SubmitSprite(view, Texture("rider_shadow"), corners, 0.0f, 0.0f, 1.0f, 1.0f, 0xB0FFFFFFu, 0, BGFX_SAMPLER_UVW_CLAMP);
	}

	const World::RiderPose pose = animation.GetPose();
	for (const World::SpriteLayer& layer : sprite.layers)
	{
		const RoleTexture& texture = Texture(layer.role);
		// Layer rectangle in frame pixels.
		float left   = static_cast<float>(layer.rectX);
		float top    = static_cast<float>(layer.rectY);
		float right  = layer.rectWidth > 0 ? left + layer.rectWidth : static_cast<float>(sprite.frameWidth);
		float bottom = layer.rectHeight > 0 ? top + layer.rectHeight : static_cast<float>(sprite.frameHeight);
		if (pose == World::RiderPose::OUT_OF_SADDLE && layer.motion == World::SpriteLayerMotion::STATIC)
		{
			top += layer.outOfSaddleOffsetY;
			bottom += layer.outOfSaddleOffsetY;
		}
		float u0 = 0.0f;
		float v0 = 0.0f;
		float u1 = 1.0f;
		float v1 = 1.0f;
		uint32_t sampler = BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP;
		float spin       = 0.0f;
		uint32_t color   = 0xFFFFFFFFu;
		if (layer.motion == World::SpriteLayerMotion::CRANK || layer.motion == World::SpriteLayerMotion::STATIC)
		{
			int column = 0;
			int row    = 0;
			if (layer.motion == World::SpriteLayerMotion::CRANK)
			{
				CrankCell(layer, sprite, pose, animation.GetCrankPhase(), column, row);
			}
			if (leanRow >= 0 && layer.motion == World::SpriteLayerMotion::CRANK)
			{
				row = leanRow;
			}
			column = std::clamp(column, 0, texture.columns - 1);
			row    = std::clamp(row, 0, texture.rows - 1);
			u0     = static_cast<float>(column) / texture.columns;
			u1     = static_cast<float>(column + 1) / texture.columns;
			v0     = static_cast<float>(row) / texture.rows;
			v1     = static_cast<float>(row + 1) / texture.rows;
		}
		else if (layer.motion == World::SpriteLayerMotion::WHEEL_SCROLL)
		{
			// Tread strip scrolls down as the wheel turns (rear view): 4 texture repeats per revolution.
			const float scroll = static_cast<float>(animation.GetWheelAngle() / (2.0 * PI) * 4.0);
			v0      = -scroll;
			v1      = v0 + (bottom - top) / static_cast<float>(texture.height);
			sampler = BGFX_SAMPLER_POINT;  // repeat
		}
		else
		{
			spin = static_cast<float>(animation.GetWheelAngle());
			if (World::WheelAliases(animation.GetWheelRevolutionsPerSecond(), layer.spokes, 60.0))
			{
				color = 0x80FFFFFFu;  // spokes would strobe: fade towards a blurred wheel
			}
		}
		// Frame-pixel rectangle -> screen, rotated about the anchor by the lean (and about its centre by the spin).
		const float centreX = (left + right) * 0.5f;
		const float centreY = (top + bottom) * 0.5f;
		const float frameX[4] = {left, right, right, left};
		const float frameY[4] = {top, top, bottom, bottom};
		float corners[8];
		for (int corner = 0; corner < 4; ++corner)
		{
			float pointX = frameX[corner];
			float pointY = frameY[corner];
			if (spin != 0.0f)
			{
				const float dx = pointX - centreX;
				const float dy = pointY - centreY;
				pointX         = centreX + dx * std::cos(spin) - dy * std::sin(spin);
				pointY         = centreY + dx * std::sin(spin) + dy * std::cos(spin);
			}
			const float offsetX = (pointX - sprite.anchorX) * scale;
			const float offsetY = (pointY - sprite.anchorY) * scale;
			corners[corner * 2]     = anchorX + offsetX * cosine + offsetY * sine;
			corners[corner * 2 + 1] = anchorY - offsetX * sine + offsetY * cosine;
		}
		SubmitSprite(view, texture, corners, u0, v0, u1, v1, color, 0, sampler);
	}
}

void RideWorldRenderer::DrawPost(const World::World& world, bgfx::FrameBufferHandle target, uint16_t width,
                                 uint16_t height)
{
	const bgfx::ViewId view = GraphicsDevice::RidePostViewId;
	const glm::mat4 ortho   = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
	const glm::mat4 identity(1.0f);
	bgfx::setViewName(view, "Ride post");
	bgfx::setViewFrameBuffer(view, target);
	bgfx::setViewRect(view, 0, 0, width, height);
	bgfx::setViewClear(view, BGFX_CLEAR_NONE);
	bgfx::setViewTransform(view, glm::value_ptr(identity), glm::value_ptr(ortho));

	const World::WorldSettings& settings = world.GetSettings();
	SetVec4("u_post", static_cast<float>(m_frameWidth), static_cast<float>(m_frameHeight), static_cast<float>(settings.paletteLevels),
	        settings.dither ? 1.0f : 0.0f);
	SetVec4("u_post2", settings.scanlines ? 1.0f : 0.0f, 0.35f, 0.0f, 0.0f);
	const bool flip = bgfx::getCaps()->originBottomLeft;
	ScreenVertex quad[4];
	ScreenQuad(static_cast<float>(width), static_cast<float>(height), 0.0f, flip ? 1.0f : 0.0f, 1.0f, flip ? 0.0f : 1.0f, quad);
	if (SubmitScreenQuad(m_screenLayout, quad))
	{
		const uint32_t sampler = settings.nearestUpscale ? POINT_CLAMP : static_cast<uint32_t>(BGFX_SAMPLER_UVW_CLAMP);
		bgfx::setTexture(0, Uniform("s_scene", bgfx::UniformType::Sampler), m_sceneColor, sampler);
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
		bgfx::submit(view, GetEmbeddedProgram("ridePost"));
	}
}

void RideWorldRenderer::DrawDebugOverlay(World::World& world, float frameSeconds)
{
	if (!world.GetSettings().debugOverlay)
	{
		if (m_debugTextEnabled)
		{
			bgfx::setDebug(BGFX_DEBUG_NONE);
			m_debugTextEnabled = false;
		}
		return;
	}
	if (!m_debugTextEnabled)
	{
		bgfx::setDebug(BGFX_DEBUG_TEXT);
		m_debugTextEnabled = true;
	}
	const bgfx::Stats* stats                = bgfx::getStats();
	const World::StreamerStats streamer = world.GetStreamer().GetStats();
	const World::BiomeBlend blend       = world.GetBlend();
	const std::vector<World::BiomeDefinition>& biomes = world.GetBiomeSequence().GetBiomes();
	const char* from = biomes.empty() ? "-" : biomes[static_cast<size_t>(blend.from)].id.c_str();
	const char* to   = biomes.empty() ? "-" : biomes[static_cast<size_t>(blend.to)].id.c_str();
	bgfx::dbgTextClear();
	bgfx::dbgTextPrintf(1, 1, 0x0f, "Ride world  frame %.2f ms (%.0f fps)  draws %u  prims %u", frameSeconds * 1000.0f,
	                    frameSeconds > 0.0f ? 1.0f / frameSeconds : 0.0f, stats->numDraw,
	                    stats->numPrims[bgfx::Topology::TriList]);
	bgfx::dbgTextPrintf(1, 2, 0x0f, "chunks %d ready, %d pending, %d pooled  build last %.2f / avg %.2f / max %.2f ms",
	                    streamer.readyChunks, streamer.pendingJobs, streamer.pooledMeshes, streamer.lastBuildMilliseconds,
	                    streamer.averageBuildMilliseconds, streamer.maxBuildMilliseconds);
	bgfx::dbgTextPrintf(1, 4, 0x0f, "props %u draws, %u instances  gpu chunks %d", m_propDraws, m_propInstances,
	                    static_cast<int>(m_chunks.size()));
	bgfx::dbgTextPrintf(1, 3, 0x0f, "biome %s -> %s (%.2f)  s %.0f m  grade %.1f%%  scene %ux%u  quality %s", from, to,
	                    blend.weight, world.GetDistance(), world.GetRoadGradePercent(), m_frameWidth, m_frameHeight,
	                    World::ToString(world.GetSettings().quality));
}

void RideWorldRenderer::Render(GameContext& context, World::World& world, float frameSeconds)
{
	if (!m_initialised)
	{
		return;
	}
	uint16_t targetWidth  = 0;
	uint16_t targetHeight = 0;
	const bgfx::FrameBufferHandle target = context.GetGraphicsDevice().GetWorldTarget(targetWidth, targetHeight);
	if (targetWidth == 0 || targetHeight == 0)
	{
		return;
	}
	const World::QualityParameters quality = world.GetSettings().Resolved();
	const uint16_t lowHeight = static_cast<uint16_t>(std::clamp<int>(quality.lowResHeight, 90, targetHeight));
	const uint16_t lowWidth  = static_cast<uint16_t>(std::clamp<long>(
        std::lround(static_cast<double>(lowHeight) * targetWidth / targetHeight), 16, 2048));
	EnsureFrameBuffer(lowWidth, lowHeight);
	SyncChunks(world);
	UpdateHeightTexture(world);

	// Camera in the render frame: x/z relative to the camera (precision over endless distances), y relative to h(rider).
	const World::WorldCamera& camera = world.GetCamera();
	m_cameraX = camera.x;
	m_cameraZ = camera.z;
	const glm::vec3 eye(0.0f, static_cast<float>(camera.y), 0.0f);
	const glm::vec3 target3(static_cast<float>(camera.targetX - camera.x), static_cast<float>(camera.targetY),
	                        static_cast<float>(camera.targetZ - camera.z));
	const glm::vec3 up(0.0f, 1.0f, 0.0f);
	const glm::mat4 view = glm::lookAtRH(eye, target3, up);
	const float aspect   = static_cast<float>(lowWidth) / static_cast<float>(lowHeight);
	const float fov      = static_cast<float>(camera.fieldOfViewDegrees) * PI / 180.0f;
	const float farPlane = static_cast<float>(quality.drawDistanceMeters + 200.0);
	const glm::mat4 projection = bgfx::getCaps()->homogeneousDepth ? glm::perspectiveRH_NO(fov, aspect, NEAR_PLANE, farPlane)
	                                                               : glm::perspectiveRH_ZO(fov, aspect, NEAR_PLANE, farPlane);
	const glm::mat4 rotationView = glm::lookAtRH(glm::vec3(0.0f), target3 - eye, up);
	std::memcpy(m_view, glm::value_ptr(view), sizeof(m_view));
	std::memcpy(m_projection, glm::value_ptr(projection), sizeof(m_projection));
	const glm::mat4 rotationViewProjection = projection * rotationView;
	std::memcpy(m_rotationViewProjection, glm::value_ptr(rotationViewProjection), sizeof(m_rotationViewProjection));
	m_eye[0] = eye.x;
	m_eye[1] = eye.y;
	m_eye[2] = eye.z;
	glm::vec3 forward = target3 - eye;
	forward.y         = 0.0f;
	const glm::vec3 right = glm::normalize(glm::cross(glm::length(forward) > 0.0f ? glm::normalize(forward) : glm::vec3(1, 0, 0), up));
	m_right[0] = right.x;
	m_right[1] = right.y;
	m_right[2] = right.z;

	// Sun (or moon) for lighting: height from the time of day.
	const World::BiomeLook& look = world.GetLook();
	const float hours                = static_cast<float>(look.timeOfDay);
	const float sunElevation         = std::sin((hours - 6.0f) / 12.0f * PI) * 62.0f;
	glm::vec3 light;
	if (sunElevation > 2.0f && look.starIntensity < 0.5)
	{
		light         = SkyDirection(sunElevation, static_cast<float>(look.sunAzimuthDeg));
		m_sunStrength = std::clamp(std::sin(sunElevation * PI / 180.0f) * 1.6f, 0.3f, 1.0f);
	}
	else
	{
		light         = SkyDirection(sunElevation > 2.0f ? sunElevation : 32.0f, static_cast<float>(look.sunAzimuthDeg) + 150.0f);
		m_sunStrength = static_cast<float>(0.12 + 0.2 * look.moonIntensity);
	}
	m_sunDirection[0] = light.x;
	m_sunDirection[1] = light.y;
	m_sunDirection[2] = light.z;

	// Rider anchor: the road point under the rider (height 0 = h(rider)) and the top of a heightMeters-tall sprite.
	{
		const World::RouteSample rider = world.GetRoute().SampleAt(world.GetDistance());
		const glm::mat4 viewProjection     = projection * view;
		const glm::vec3 foot(static_cast<float>(rider.x - m_cameraX), 0.0f, static_cast<float>(rider.z - m_cameraZ));
		const glm::vec4 footClip = viewProjection * glm::vec4(foot, 1.0f);
		const glm::vec4 headClip = viewProjection * glm::vec4(foot + glm::vec3(0.0f, static_cast<float>(m_descriptor.heightMeters), 0.0f), 1.0f);
		m_riderPixelHeight = 0.0f;
		if (footClip.w > 0.1f && headClip.w > 0.1f)
		{
			const float footY  = (1.0f - (footClip.y / footClip.w * 0.5f + 0.5f)) * lowHeight;
			const float headY  = (1.0f - (headClip.y / headClip.w * 0.5f + 0.5f)) * lowHeight;
			m_riderAnchorX     = (footClip.x / footClip.w * 0.5f + 0.5f) * lowWidth;
			m_riderAnchorY     = footY;
			m_riderPixelHeight = footY - headY;
		}
	}
	m_propDraws     = 0;
	m_propInstances = 0;

	SetSharedUniforms(world);
	DrawSky(world);
	DrawScene(world);
	DrawRider(world);
	DrawPost(world, target, targetWidth, targetHeight);
	DrawDebugOverlay(world, frameSeconds);
}
}  // namespace Struktur::Renderer
