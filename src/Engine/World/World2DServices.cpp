#include "World2DServices.h"

#include "Engine/Renderer/EmbeddedShaders.h"

namespace Struktur::World
{
Services::Services()
{
	// Custom entity shaders (Component::Shader) are looked up by name in the embedded tables.
	Renderer::RegisterEmbeddedShaders(Shaders::World2DShaders());
	camera         = std::make_unique<Camera2D>();
	spriteRenderer = std::make_unique<Renderer::SpriteBatchRenderer>();
}

void Services::Shutdown()
{
	spriteRenderer->Shutdown();  // while the graphics device is still up
	spriteRenderer.reset();
	camera.reset();
}
}  // namespace Struktur::World
