#pragma once

#include <functional>
#include <string>
#include <vector>

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
	Window() = default;
	~Window();

	Window(const Window&)            = delete;
	Window& operator=(const Window&) = delete;

	// Actually creates the SDL window - deferred from construction so GameContext can own this unconditionally
	// (see GameContext's constructor) while the caller still picks when the real window size/title are known.
	void Initialise(int width, int height, const std::string& title, bool resizable);

	// Pumps SDL events, updates the keyboard/mouse snapshot Input reads, and tracks close/resize requests.
	void PollEvents();

	// Called for every raw SDL_Event seen by PollEvents(), before Window's own handling - lets the editor feed
	// events to ImGui_ImplSDL3_ProcessEvent (which needs the actual event stream: text input, wheel, focus, not
	// just polled state) and, separately, Input::HandleEvent see touch-finger events (SDL3 has no pollable
	// touch-state function the way keyboard/mouse have). Multiple independent callbacks rather than one slot -
	// both of the above need to observe the same single SDL_PollEvent drain each frame; calling PollEvents()
	// twice per frame would silently starve whichever caller polled second, since SDL_PollEvent() drains the
	// queue as it goes.
	void AddEventCallback(std::function<void(const SDL_Event&)> callback)
	{
		m_eventCallbacks.push_back(std::move(callback));
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
	std::vector<std::function<void(const SDL_Event&)>> m_eventCallbacks;
	int m_width         = 0;
	int m_height        = 0;
	bool m_isFullscreen = false;
	bool m_shouldClose  = false;
	bool m_wasResized   = false;
};
}  // namespace Platform
}  // namespace Struktur
