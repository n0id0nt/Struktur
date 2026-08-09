#pragma once

#include <functional>
#include <string>

struct SDL_Window;
union SDL_Event;

namespace Struktur
{
namespace Platform
{
// Owns the OS window on desktop via SDL3 directly - replaces raylib's ::InitWindow()/::CloseWindow(), since bgfx
// owns rendering now and doesn't need (or want) raylib's GL-context-creating window layer underneath it.
class Window
{
   public:
	Window(int width, int height, const std::string& title, bool resizable);
	~Window();

	Window(const Window&)            = delete;
	Window& operator=(const Window&) = delete;

	// Pumps SDL events, updates the keyboard/mouse snapshot Input reads, and tracks close/resize requests.
	void PollEvents();

	// Called for every raw SDL_Event seen by PollEvents(), before Window's own handling - lets the editor feed
	// events to ImGui_ImplSDL3_ProcessEvent, which needs the actual event stream (text input, wheel, focus),
	// not just polled state.
	void SetEventCallback(std::function<void(const SDL_Event&)> callback)
	{
		m_eventCallback = std::move(callback);
	}

	bool ShouldClose() const
	{
		return m_shouldClose;
	}
	bool WasResized() const
	{
		return m_wasResized;
	}

	int GetWidth() const
	{
		return m_width;
	}
	int GetHeight() const
	{
		return m_height;
	}

	void SetFullscreen(bool fullscreen);
	bool IsFullscreen() const
	{
		return m_isFullscreen;
	}

	// The OS-native handle bgfx::PlatformData::nwh needs (HWND/X11 Window/NSWindow) - NOT the SDL_Window* itself.
	void* GetNativeHandle() const;

	SDL_Window* GetSDLWindow() const
	{
		return m_window;
	}

   private:
	SDL_Window* m_window = nullptr;
	std::function<void(const SDL_Event&)> m_eventCallback;
	int m_width           = 0;
	int m_height          = 0;
	bool m_isFullscreen   = false;
	bool m_shouldClose    = false;
	bool m_wasResized     = false;
};
}  // namespace Platform
}  // namespace Struktur
