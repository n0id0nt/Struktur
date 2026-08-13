#include "GraphicsDevice.h"

#include <format>

#include "Debug/Assertions.h"
#include "Engine/Renderer/EmbeddedShaders.h"

// bgfx::PlatformData is declared directly in bgfx.h in this version - no separate platform.h needed.
#include <bgfx/bgfx.h>

void Struktur::Renderer::GraphicsDevice::Initialise(void* nativeWindowHandle, int width, int height)
{
	m_width  = width;
	m_height = height;

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

	// No clear - composites on top of whatever WorldViewId already drew into the same target. Default (state-
	// sorted) ViewMode is fine here - debug shapes have no ordering requirement among themselves, only relative
	// to WorldViewId (below) and UIViewId (above), both already guaranteed by view ID order.
	bgfx::setViewClear(DebugViewId, BGFX_CLEAR_NONE);
	bgfx::setViewRect(DebugViewId, 0, 0, (uint16_t)width, (uint16_t)height);

	// No clear - composites on top of whatever WorldViewId/DebugViewId already drew into the same target.
	// Sequential mode preserves the UI tree's z-index paint order instead of letting bgfx reorder by state.
	bgfx::setViewClear(UIViewId, BGFX_CLEAR_NONE);
	bgfx::setViewRect(UIViewId, 0, 0, (uint16_t)width, (uint16_t)height);
	bgfx::setViewMode(UIViewId, bgfx::ViewMode::Sequential);

#ifdef EDITOR
	// No clear - it composites on top of whatever WorldViewId/DebugViewId/UIViewId already drew. Sequential mode
	// preserves ImGui's own draw-command order instead of letting bgfx reorder by state (the default for opaque
	// geometry).
	bgfx::setViewClear(EditorViewId, BGFX_CLEAR_NONE);
	bgfx::setViewRect(EditorViewId, 0, 0, (uint16_t)width, (uint16_t)height);
	bgfx::setViewMode(EditorViewId, bgfx::ViewMode::Sequential);
#endif

	DEBUG_INFO(std::format("bgfx initialised, renderer: {}", bgfx::getRendererName(bgfx::getRendererType())).c_str());
	m_initialised = true;
}

void Struktur::Renderer::GraphicsDevice::Shutdown()
{
	if (m_initialised)
	{
		bgfx::shutdown();
		m_initialised = false;
	}
}

void Struktur::Renderer::GraphicsDevice::Resize(int width, int height)
{
	m_width  = width;
	m_height = height;
	// bgfx::reset() unconditionally clears every view's frame buffer binding back to the backbuffer as a side
	// effect (see bgfx's Context::reset, m_view[ii].setFrameBuffer(BGFX_INVALID_HANDLE) for every view) -
	// silently undoing SetWorldRenderTarget's redirect into the editor's Game Viewport framebuffer. Left
	// unhandled, World/Debug/UI keep drawing every frame, just now straight to the backbuffer instead of that
	// framebuffer - which then never receives another draw, so the Game Viewport panel's ImGui::Image freezes
	// on whatever was last rendered before the resize. Re-apply whichever target was actually active.
	bgfx::reset((uint32_t)width, (uint32_t)height, BGFX_RESET_VSYNC);

	if (m_worldRenderTargetActive)
	{
		RestoreWorldRenderTarget();
	}
	else
	{
		// Not redirected (release/web builds, or mid-splash-bypass) - World/Debug/UI draw straight to the
		// backbuffer, so their rect should track the real window size like EditorViewId's does below.
		bgfx::setViewRect(WorldViewId, 0, 0, (uint16_t)width, (uint16_t)height);
		bgfx::setViewRect(DebugViewId, 0, 0, (uint16_t)width, (uint16_t)height);
		bgfx::setViewRect(UIViewId, 0, 0, (uint16_t)width, (uint16_t)height);
	}
#ifdef EDITOR
	bgfx::setViewRect(EditorViewId, 0, 0, (uint16_t)width, (uint16_t)height);
#endif
}

void Struktur::Renderer::GraphicsDevice::SetWorldRenderTarget(bgfx::FrameBufferHandle frameBuffer, uint16_t width,
                                                               uint16_t height)
{
	m_worldFrameBuffer       = frameBuffer;
	m_worldFrameBufferWidth  = width;
	m_worldFrameBufferHeight = height;
	RestoreWorldRenderTarget();
}

void Struktur::Renderer::GraphicsDevice::ResetWorldRenderTarget()
{
	m_worldRenderTargetActive = false;
	bgfx::setViewFrameBuffer(WorldViewId, BGFX_INVALID_HANDLE);
	bgfx::setViewFrameBuffer(DebugViewId, BGFX_INVALID_HANDLE);
	bgfx::setViewFrameBuffer(UIViewId, BGFX_INVALID_HANDLE);
	// Drawing straight to the backbuffer now (e.g. SplashScreenLoop's bypass) - match its real size, not
	// whatever the offscreen framebuffer's rect was.
	bgfx::setViewRect(WorldViewId, 0, 0, (uint16_t)m_width, (uint16_t)m_height);
	bgfx::setViewRect(DebugViewId, 0, 0, (uint16_t)m_width, (uint16_t)m_height);
	bgfx::setViewRect(UIViewId, 0, 0, (uint16_t)m_width, (uint16_t)m_height);
}

void Struktur::Renderer::GraphicsDevice::RestoreWorldRenderTarget()
{
	if (bgfx::isValid(m_worldFrameBuffer))
	{
		m_worldRenderTargetActive = true;
		bgfx::setViewFrameBuffer(WorldViewId, m_worldFrameBuffer);
		bgfx::setViewFrameBuffer(DebugViewId, m_worldFrameBuffer);
		bgfx::setViewFrameBuffer(UIViewId, m_worldFrameBuffer);
		bgfx::setViewRect(WorldViewId, 0, 0, m_worldFrameBufferWidth, m_worldFrameBufferHeight);
		bgfx::setViewRect(DebugViewId, 0, 0, m_worldFrameBufferWidth, m_worldFrameBufferHeight);
		bgfx::setViewRect(UIViewId, 0, 0, m_worldFrameBufferWidth, m_worldFrameBufferHeight);
	}
}

void Struktur::Renderer::GraphicsDevice::BeginFrame()
{
	// Ensures every view gets cleared/participates in the frame even when nothing is submitted to it.
	bgfx::touch(WorldViewId);
	bgfx::touch(DebugViewId);
	bgfx::touch(UIViewId);
}

void Struktur::Renderer::GraphicsDevice::EndFrame()
{
	bgfx::frame();
}

bgfx::ProgramHandle Struktur::Renderer::GraphicsDevice::GetDefaultSpriteProgram() const
{
	return GetEmbeddedProgram("sprite");
}
