#pragma once

#include <memory>

#include "Engine/Renderer/SpriteBatchRenderer.h"
#include "Engine/World/Camera2D.h"

namespace Struktur::World
{
// The 2D world library's services. A game's context holds one and returns its members.
struct Services
{
	Services();
	// Destroys the services, in order.
	void Shutdown();

	std::unique_ptr<Camera2D> camera;
	std::unique_ptr<Renderer::SpriteBatchRenderer> spriteRenderer;
};
}  // namespace Struktur::World
