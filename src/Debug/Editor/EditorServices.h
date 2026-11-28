#pragma once

#include <memory>

#include "Debug/Editor/Editor.h"
#include "Engine/Renderer/ImGuiRenderer.h"

namespace Struktur::Debug
{
// The editor library's services. A game's context holds one and returns its members.
struct Services
{
	Services();
	// Destroys the services, in order.
	void Shutdown();

	std::unique_ptr<Editor> editor;
	std::unique_ptr<Renderer::ImGuiRenderer> imGuiRenderer;
};
}  // namespace Struktur::Debug
