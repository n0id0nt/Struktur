#pragma once

#include <memory>

#include "Engine/ECS/EcsServices.h"
#include "Engine/Fitness/FitnessServices.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenServices.h"
#include "Engine/UI/UIServices.h"
#ifdef EDITOR
	#include "Debug/Editor/EditorServices.h"
#endif

namespace Struktur
{
namespace RideIntegration
{
class RideService;
}
namespace World
{
class Camera;
class World;
}  // namespace World
namespace Renderer
{
class WorldRenderer;
}

// The indoor cycling game's context: the libraries it uses, plus the ride, the road world and its camera and renderer.
// Everything is created in the constructor and destroyed in Shutdown.
class RideContext final : public GameContext
{
public:
	RideContext();
	~RideContext() override;

	// Safe from anywhere in the game: a program has one kind of context.
	static RideContext& From(GameContext& context)
	{
		return static_cast<RideContext&>(context);
	}

	void Shutdown() override;

	// --- the game's own ------------------------------------------------------------------------------------------
	RideIntegration::RideService& GetRideService() const;
	World::World& GetWorld() const;
	World::Camera& GetCamera() const;
	Renderer::WorldRenderer& GetWorldRenderer() const;

	// --- the libraries' ------------------------------------------------------------------------------------------
	entt::registry& GetRegistry() const override;
	System::GameObjectManager& GetGameObjectManager() const override;
	System::SystemManager& GetSystemManager() const override;
	Wren::WrenScriptComponentRegistry& GetWrenScriptComponentRegistry() const override;
	Wren::WrenScriptEngine& GetWrenScriptEngine() const override;
	Wren::WrenStateManager& GetWrenStateManager() const override;
	UI::InputGlyphs& GetInputGlyphs() const override;
	UI::UIManager& GetUIManager() const override;
	Renderer::UIRenderer& GetUIRenderer() const override;
	Fitness::FitnessManager& GetFitnessManager() const override;
#ifdef EDITOR
	Debug::Editor& GetEditor() const override;
	Renderer::ImGuiRenderer& GetImGuiRenderer() const override;
#endif

private:
	System::Services m_ecs;
	Wren::Services m_wren;
	UI::Services m_ui;
	Fitness::Services m_fitness;
#ifdef EDITOR
	Debug::Services m_editor;
#endif

	std::unique_ptr<World::Camera> m_camera;
	std::unique_ptr<RideIntegration::RideService> m_rideService;
	std::unique_ptr<World::World> m_world;
	std::unique_ptr<Renderer::WorldRenderer> m_worldRenderer;
};
}  // namespace Struktur
