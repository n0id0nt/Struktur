#include "Window.h"

#include <SDL3/SDL.h>

#include "Debug/Assertions.h"

void Struktur::Platform::Window::Initialise(int width, int height, const std::string& title, bool resizable)
{
	m_width  = width;
	m_height = height;

	Uint32 flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
	if (resizable)
	{
		flags |= SDL_WINDOW_RESIZABLE;
	}

	m_window = ::SDL_CreateWindow(title.c_str(), width, height, flags);
	ASSERT_MSG(m_window, "Failed to create SDL window: %s", ::SDL_GetError());

	DEBUG_INFO("Created SDL3 window (bgfx renders into it - no GL context requested).");
}

Struktur::Platform::Window::~Window()
{
	if (m_window)
	{
		::SDL_DestroyWindow(m_window);
		m_window = nullptr;
	}
}

void Struktur::Platform::Window::PollEvents()
{
	m_wasResized = false;

	SDL_Event event;
	while (::SDL_PollEvent(&event))
	{
		for (const auto& callback : m_eventCallbacks)
		{
			callback(event);
		}

		switch (event.type)
		{
			case SDL_EVENT_QUIT:
				m_shouldClose = true;
				break;
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				m_shouldClose = true;
				break;
			case SDL_EVENT_WINDOW_RESIZED:
				m_width      = event.window.data1;
				m_height     = event.window.data2;
				m_wasResized = true;
				break;
			default:
				break;
		}
	}
}

void Struktur::Platform::Window::SetFullscreen(bool fullscreen)
{
	::SDL_SetWindowFullscreen(m_window, fullscreen);
	m_isFullscreen = fullscreen;
}

void* Struktur::Platform::Window::GetNativeHandle() const
{
#if defined(__EMSCRIPTEN__)
	// bgfx's Emscripten backend expects PlatformData::nwh to be a pointer to a CSS selector string identifying
	// the target canvas (see web/shell.html's <canvas id="canvas">), not a real platform window handle - there
	// is no X11/Win32/Cocoa window object to hand it under Emscripten.
	return (void*)"#canvas";
#else
	SDL_PropertiesID props = ::SDL_GetWindowProperties(m_window);
	#if defined(_WIN32)
	return ::SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
	#elif defined(__APPLE__)
	return ::SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
	#else
	// X11 - Wayland-only sessions have no numbered window id here; cross that bridge if/when Linux is targeted.
	return reinterpret_cast<void*>(::SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
	#endif
#endif
}
