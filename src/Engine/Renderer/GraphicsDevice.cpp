#include "GraphicsDevice.h"

#include <format>

#include "Debug/Assertions.h"
#include "Engine/Renderer/EmbeddedShaders.h"

// bgfx::PlatformData is declared directly in bgfx.h in this version - no separate platform.h needed.
#include <bgfx/bgfx.h>

Struktur::Renderer::GraphicsDevice::GraphicsDevice(void* nativeWindowHandle, int width, int height)
    : m_width(width),
      m_height(height)
{
	bgfx::PlatformData platformData;
	platformData.nwh = nativeWindowHandle;

	bgfx::Init init;
	init.type             = bgfx::RendererType::Count;  // auto-select the platform's preferred backend
	init.resolution.width  = (uint32_t)width;
	init.resolution.height = (uint32_t)height;
	init.resolution.reset  = BGFX_RESET_VSYNC;
	init.platformData      = platformData;

	bool ok = bgfx::init(init);
	ASSERT_MSG(ok, "bgfx::init failed");

	bgfx::setViewClear(WorldViewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
	bgfx::setViewRect(WorldViewId, 0, 0, (uint16_t)width, (uint16_t)height);

	// No clear - it composites on top of whatever WorldViewId already drew. Sequential mode preserves ImGui's
	// own draw-command order instead of letting bgfx reorder by state (the default for opaque-geometry views).
	bgfx::setViewClear(EditorViewId, BGFX_CLEAR_NONE);
	bgfx::setViewRect(EditorViewId, 0, 0, (uint16_t)width, (uint16_t)height);
	bgfx::setViewMode(EditorViewId, bgfx::ViewMode::Sequential);

	DEBUG_INFO(std::format("bgfx initialised, renderer: {}", bgfx::getRendererName(bgfx::getRendererType())).c_str());
}

Struktur::Renderer::GraphicsDevice::~GraphicsDevice()
{
	bgfx::shutdown();
}

void Struktur::Renderer::GraphicsDevice::Resize(int width, int height)
{
	m_width  = width;
	m_height = height;
	bgfx::reset((uint32_t)width, (uint32_t)height, BGFX_RESET_VSYNC);
	bgfx::setViewRect(WorldViewId, 0, 0, (uint16_t)width, (uint16_t)height);
	bgfx::setViewRect(EditorViewId, 0, 0, (uint16_t)width, (uint16_t)height);
}

void Struktur::Renderer::GraphicsDevice::SetWorldRenderTarget(bgfx::FrameBufferHandle frameBuffer)
{
	bgfx::setViewFrameBuffer(WorldViewId, frameBuffer);
}

void Struktur::Renderer::GraphicsDevice::ResetWorldRenderTarget()
{
	bgfx::setViewFrameBuffer(WorldViewId, BGFX_INVALID_HANDLE);
}

void Struktur::Renderer::GraphicsDevice::BeginFrame()
{
	// Ensures the view gets cleared even on a frame where nothing is submitted to it.
	bgfx::touch(WorldViewId);
}

void Struktur::Renderer::GraphicsDevice::EndFrame()
{
	bgfx::frame();
}

bgfx::ProgramHandle Struktur::Renderer::GraphicsDevice::GetDefaultSpriteProgram() const
{
	return GetEmbeddedProgram("sprite");
}
