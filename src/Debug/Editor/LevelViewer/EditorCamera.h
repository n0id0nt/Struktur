#pragma once

#include "glm/glm.hpp"

namespace Struktur
{
class GameContext;

namespace Debug
{
class GameViewportWindow;

// Free-flying camera for the level viewer: drag with the right or middle mouse button to pan, scroll to zoom
// around the cursor. Writes World::Camera directly - CameraSystem is an Update system, so it leaves the camera
// alone while the game is stopped.
class EditorCamera
{
public:
	struct View
	{
		glm::vec2 target{0.f};
		float zoom = 1.f;
	};

	// Reads this frame's mouse input over the Game Viewport.
	void HandleInput(GameContext& context, const GameViewportWindow& viewport);
	// Writes the view into context.GetCamera(), centred on the current game resolution.
	void Apply(GameContext& context) const;

	// Fits a world-space rect (px) into a view of viewSize game pixels, with a small margin.
	void FrameRect(const glm::vec2& position, const glm::vec2& size, const glm::vec2& viewSize);

	const View& GetView() const
	{
		return m_view;
	}
	void SetView(const View& view)
	{
		m_view = view;
	}

private:
	View m_view;
	// Set while a pan drag that started over the viewport is held, so the drag keeps going if the cursor leaves it.
	bool m_isPanning = false;
};
}  // namespace Debug
}  // namespace Struktur
