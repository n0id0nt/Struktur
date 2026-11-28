#include "GraphicsDevice.h"

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
	init.type              = bgfx::RendererType::Count;  // auto-select the platform's preferred backend
	init.resolution.width  = (uint32_t)width;
	init.resolution.height = (uint32_t)height;
	init.resolution.reset  = m_resetFlags;
	init.platformData      = platformData;

	bool ok = bgfx::init(init);
	ASSERT_MSG(ok, "bgfx::init failed");

	RegisterEmbeddedShaders(Shaders::CoreShaders());

	bgfx::setViewClear(WorldViewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
	bgfx::setViewRect(WorldViewId, 0, 0, (uint16_t)width, (uint16_t)height);
	m_views = {{WorldViewId, ViewTarget::Game}};

	DEBUG_INFO("bgfx initialised, renderer: %s", bgfx::getRendererName(bgfx::getRendererType()));
	m_initialised = true;
}

void Struktur::Renderer::GraphicsDevice::Shutdown()
{
	if (m_initialised)
	{
		// Process-lifetime shader/program/uniform cache (see EmbeddedShaders.h) - has no other owner to call this
		// from, and must run before bgfx::shutdown() below like everything else here. Fully qualified: an
		// unqualified call here would resolve to this very function (GraphicsDevice::Shutdown), not the free
		// Renderer::Shutdown from EmbeddedShaders.h.
		Struktur::Renderer::Shutdown();
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
	bgfx::reset((uint32_t)width, (uint32_t)height, m_resetFlags);

	if (m_worldRenderTargetActive)
	{
		RestoreWorldRenderTarget();
	}
	else
	{
		// Not redirected (release/web builds, or mid-splash-bypass) - the game's views draw straight to the
		// backbuffer, so their rect should track the real window size.
		BindGameViews(BGFX_INVALID_HANDLE, (uint16_t)width, (uint16_t)height);
	}
	for (const View& view : m_views)
	{
		if (view.target == ViewTarget::Window)
		{
			bgfx::setViewRect(view.id, 0, 0, (uint16_t)width, (uint16_t)height);
		}
	}
}

bgfx::ViewId Struktur::Renderer::GraphicsDevice::AddView(ViewTarget target, bool sequential)
{
	ASSERT_MSG(m_initialised, "AddView needs the graphics device to be initialised");
	// bgfx runs views in ascending id order, so handing ids out in call order gives the draw order.
	const bgfx::ViewId id = static_cast<bgfx::ViewId>(m_views.size());
	m_views.push_back({id, target});

	bgfx::setViewClear(id, BGFX_CLEAR_NONE);
	if (sequential)
	{
		bgfx::setViewMode(id, bgfx::ViewMode::Sequential);
	}
	if (target == ViewTarget::Game && m_worldRenderTargetActive)
	{
		RestoreWorldRenderTarget();
	}
	else if (target != ViewTarget::Own)
	{
		bgfx::setViewRect(id, 0, 0, (uint16_t)m_width, (uint16_t)m_height);
	}
	return id;
}

void Struktur::Renderer::GraphicsDevice::BindGameViews(bgfx::FrameBufferHandle frameBuffer, uint16_t width,
                                                       uint16_t height)
{
	for (const View& view : m_views)
	{
		if (view.target == ViewTarget::Game)
		{
			bgfx::setViewFrameBuffer(view.id, frameBuffer);
			bgfx::setViewRect(view.id, 0, 0, width, height);
		}
	}
}

void Struktur::Renderer::GraphicsDevice::SetVSync(bool enabled)
{
	if (enabled)
	{
		m_resetFlags |= BGFX_RESET_VSYNC;
	}
	else
	{
		m_resetFlags &= ~BGFX_RESET_VSYNC;
	}

	if (m_initialised)
	{
		// Reuses Resize's own bgfx::reset + render-target-reapply dance (see its comment above) instead of
		// calling bgfx::reset directly here - bgfx::reset unconditionally clears every view's frame buffer
		// binding, which would silently break the editor's Game Viewport redirect the same way an unhandled
		// window resize would.
		Resize(m_width, m_height);
	}
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
	// Drawing straight to the backbuffer now (e.g. the splash screen's bypass) - match its real size, not
	// whatever the offscreen framebuffer's rect was.
	BindGameViews(BGFX_INVALID_HANDLE, (uint16_t)m_width, (uint16_t)m_height);
}

void Struktur::Renderer::GraphicsDevice::RestoreWorldRenderTarget()
{
	if (bgfx::isValid(m_worldFrameBuffer))
	{
		m_worldRenderTargetActive = true;
		BindGameViews(m_worldFrameBuffer, m_worldFrameBufferWidth, m_worldFrameBufferHeight);
	}
}

bgfx::FrameBufferHandle Struktur::Renderer::GraphicsDevice::GetWorldTarget(uint16_t& width, uint16_t& height) const
{
	if (m_worldRenderTargetActive && bgfx::isValid(m_worldFrameBuffer))
	{
		width  = m_worldFrameBufferWidth;
		height = m_worldFrameBufferHeight;
		return m_worldFrameBuffer;
	}
	width  = static_cast<uint16_t>(m_width);
	height = static_cast<uint16_t>(m_height);
	return BGFX_INVALID_HANDLE;
}

void Struktur::Renderer::GraphicsDevice::BeginFrame()
{
	// Ensures every view gets cleared/participates in the frame even when nothing is submitted to it.
	for (const View& view : m_views)
	{
		if (view.target == ViewTarget::Game)
		{
			bgfx::touch(view.id);
		}
	}
}

void Struktur::Renderer::GraphicsDevice::EndFrame()
{
	bgfx::frame();
}

bgfx::ProgramHandle Struktur::Renderer::GraphicsDevice::GetDefaultSpriteProgram() const
{
	return GetEmbeddedProgram("sprite");
}
